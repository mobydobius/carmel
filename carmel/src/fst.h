#ifndef CARMEL_FST_H 
#define CARMEL_FST_H


#include <graehl/carmel/src/config.hpp>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <sstream>
#include <iostream>
#include <graehl/shared/2hash.h>
#include <graehl/shared/list.h>
#include <graehl/shared/weight.h>
#include <graehl/shared/strhash.h>
#include <graehl/shared/graph.h>
#include <graehl/carmel/src/train.h>
#include <graehl/shared/myassert.h>
#include <graehl/carmel/src/compose.h>
#include <iterator>
#include <graehl/shared/kbest.h>
#include <boost/config.hpp>
#include <graehl/shared/config.h>
#include <graehl/shared/mean_field_scale.hpp>

#include <graehl/shared/debugprint.hpp>

namespace graehl {

using namespace std;

class WFST;
struct PathArc {
    //const char *in;
    //const char *out;
    //const char *destState;
    int in,out,destState;
    const WFST *wfst;
    Weight weight;
};

std::ostream & operator << (std::ostream & o, const PathArc &p);



struct cascade_parameters; // in cascade.h, but we avoid circular dependency by knowing only about references in this header

class WFST {
 public:
    typedef Alphabet<StringKey,StringPool> alphabet;
 private:
    enum { STATE,ARC } PerLine;
    enum { BRIEF,FULL } ArcFormat;
    static const int perline_index; // handle to ostream iword for LogBase enum (initialized to 0)
    static const int arcformat_index; // handle for OutThresh
    
    void initAlphabet() {
#define EPSILON_SYMBOL "*e*"
        in = NEW alphabet(EPSILON_SYMBOL);
        out = NEW alphabet(EPSILON_SYMBOL);
        ownerIn=ownerOut=1;
    }
    void train_prune(); // delete states with zero counts
    void deleteAlphabet() 
    {
        deleteAlphabet(0);
        deleteAlphabet(1);
    }
    /*
    BOOST_STATIC_CONSTANT(int,input=State::input);
    BOOST_STATIC_CONSTANT(int,output=State::output);
    */
    enum {input=0,output=1};

    void deleteAlphabet(int dir) {
        if ( dir==input && ownerIn ) {
            delete in;
            ownerIn = 0;
        } else if ( dir==output && ownerOut ) {
            delete out;
            ownerOut = 0;
        }
    }

    void identity_alphabet_from(int dir) 
    {
        int to = (dir==State::input) ?
             State::output
            : State::input;
        deleteAlphabet(to);
        if (dir==input) {
            out=in;
        } else {
            in=out;
        }
    }
    
    int getStateIndex(const char *buf); // creates the state according to named_states, returns -1 on failure
 public:
    // openfst MutableFst<LogArc> or <StdArc>, eg StdVectorFst
    template <class Fst>
    void to_openfst(Fst &fst) const
    {
        typedef typename Fst::Arc A;
        typedef typename A::Weight W;
        typedef typename A::StateId I;
        fst.DeleteStates();
        for (unsigned i=0,e=numStates();i!=e;++i) {
            I s=fst.AddState();
            assert(s==i);
            states[i].to_openfst(fst,i);
        }
        fst.SetStart(0);
        fst.SetFinal(final,W::One());
    }
    template <class FstWeight>
    Weight to_weight(FstWeight const& w) 
    {
        return Weight(w.Value(),neglog10_weight());
    }

    // scoped object: on creation, turns the arcs into identity-FSA with symbol
    // = pair of old input/output.  on destruction, restores the pairs into
    // original input/output alphabet.  during the duration, any symbol<->string
    // lookup (e.g. i/o) is prohibited
    struct as_pairs_fsa
    {
        WFST &wfst;
        bool keep_epsilon; // *e*:*e* -> *e* if true, non-epsilon pair-symbol if false
//        typedef std::pair<int,int> newsym;
        typedef IOPair newsym;
        typedef std::vector<newsym> newsyms;
        newsyms syms;
        bool to_pairs;

        // does nothing if to_pairs=false
        as_pairs_fsa(WFST &wfst,bool keep_epsilon=true,bool to_pairs=true)
            :wfst(wfst)
            ,keep_epsilon(keep_epsilon) 
            ,to_pairs(to_pairs)
        {
            if (!to_pairs) return;

            typedef HashTable<newsym,unsigned> tosyms;
            
            tosyms to;

            syms.push_back(newsym(0,0));
            if (keep_epsilon)
                to.add(syms[0],0);
            
            for (StateVector::iterator i=wfst.states.begin(),e=wfst.states.end();i!=e;++i) {
                State::Arcs &arcs=i->arcs;
                for ( State::Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l ) {
                    newsym s(l->in,l->out);
                    tosyms::insert_return_type ins=to.insert(s,syms.size());
                    if (ins.second)
                        syms.push_back(s);
                    l->in=l->out=ins.first->second;
                }   
            }
        }
        ~as_pairs_fsa() 
        {
            if (!to_pairs) return;
                
            for (StateVector::iterator i=wfst.states.begin(),e=wfst.states.end();i!=e;++i) {
                State::Arcs &arcs=i->arcs;
                for ( State::Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l ) {
                    newsym &s=syms[l->in];
                    l->in=s.in;
                    l->out=s.out;
                }   
            }            
        }
            
    };        
        
    
#ifdef USE_OPENFST
    // here we depend on ArcIterator directly
    // openfst ExpandedFst<LogArc>
    template <class Fst>
    void from_openfst(Fst &fst)
    {
        typedef typename Fst::Arc A;
        typedef typename A::Weight W;
        typedef typename A::StateId I;
        typedef fst::ArcIterator<Fst> AI;
        
        const W zero=W::Zero();
        const W one=W::One();
        
        unsigned n_final=0;
        unsigned max_final=0;
        unsigned o_n=fst.NumStates();
        for (unsigned i=0;i!=o_n;++i) {
            if (zero != fst.Final(i)) {
                ++n_final;
                DBP2(i,fst.Final(i));
                max_final=i;
            }
        }
        if (n_final==0) {
            clear();
            return;
        }
        bool need_extra_final=(n_final>1 || fst.Final(max_final)!=one); // todo: if final is in no cycles, make all arcs entering final bear the final cost if not==1

        unsigned ns=o_n+(need_extra_final?1:0);
        unNameStates();
        states.clear();
        states.resize(ns);
//        DBP4(states.size(),ns,o_n,need_extra_final);
        State::arc_adder arc_add(states);
        
        unsigned o2s[o_n]; //FIXME: does an expandedfst always have ids [0...numstates)? 
        unsigned s=0,i=0;
        unsigned o_start=fst.Start();

        //... precomputing may save time but we're really just doing i==start?0:(i<start?i+1:i)
        for (unsigned o=0;o<o_n;++o) {
            o2s[o]=
                (o < o_start) ? o+1 : o;
        }
        o2s[o_start]=0;

        // actually iterate over the arcs for each state, translating ids by our constraint that start index must be 0
        
        for (unsigned o=0;o<o_n;++o) {
            for (AI ai(fst,o);!ai.Done();ai.Next()) {
                A const&arc=ai.Value();
                const FSTArc a(arc.ilabel,arc.olabel,o2s[arc.nextstate],to_weight(arc.weight));
//                DBP5(o,o2s[o],arc.nextstate,o2s[arc.nextstate],a);
                arc_add(o2s[o],a);
            }
        }

        // set final status, adding a state and arcs if necessary (carmel has no final state weights)
        if (!need_extra_final) { 
            final=max_final;
            Assert(final<numStates());
        } else {
            final=numStates()-1;
            for (unsigned i=0;i!=o_n;++i) {
                W fw=fst.Final(i);
                if (zero != fw) {
                    states[i].addArc(FSTArc(epsilon_index,epsilon_index,final,to_weight(fw)));
                }
            }
        }
        
        
    }
    
    template <class Fst>
    void roundtrip_openfst() // for debugging, to_openfst then from_openfst, will lose arc indices
    {
        Fst f;
//        DBP(numStates());
        to_openfst(f);
        from_openfst(f);
//        DBP(numStates());
    }

    // if determinize is false, input must already be deterministic.  connect=false means don't prune unconnected states.  determinize=true -> may not terminate (if lacking twins property, see openfst.org)
    template <class Fst>
    bool minimize_openfst(bool determinize=true,bool rmepsilon=false,bool minimize=true,bool connect=true,bool inverted=false,bool as_pairs=false,bool pairs_keep_epsilon=true) // for debugging, to_openfst then from_openfst, will lose arc indices
    {
        ensure_final_sink();
        if (inverted)
            invert();
        Fst f,f2;
        {
            as_pairs_fsa scoped_pairs(*this,pairs_keep_epsilon,as_pairs);
//        DBP(numStates());
//        try {
            if (determinize) {
                to_openfst(f2);
                Determinize(f2,&f);
            } else {
                to_openfst(f);
                if (!f.Properties(fst::kIDeterministic,true)) {
//                Config::log() << " (FST not input deterministic, skipping openfst minimize) ";
                    return false;
                }
            }
            if (rmepsilon) {
                RmEpsilon(&f);
            }
            if (minimize) {
                Minimize(&f);
            }
            if (connect)
                Connect(&f);
            /*
              } catch(std::exception &e) {
              Config::log() << "\nERROR: "<<e.what()<<std::endl;
              return false;
              }
            */
            from_openfst(f);
        }        
        if (inverted)
            invert();
        return true;        
//        DBP(numStates());
    }

#endif 
    enum { epsilon_index=FSTArc::epsilon };


    // some algorithms (e.g. determinizing) are confused when the final state has arcs leaving it.  this will add a new final state with no exit, if necessary (and a p=1 epsilon transition from old final state to new one)
    void ensure_final_sink() 
    {
        if (!states[final].size)
            return;
        unsigned old_final=final;
        final=add_state("FINAL_SINK");
        states[old_final].addArc(FSTArc(epsilon_index,epsilon_index,final,1));
    }
    
    
    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_state_per_line(std::basic_ostream<A,B>& os) { os.iword(perline_index) = STATE; return os; }
    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_per_line(std::basic_ostream<A,B>& os) { os.iword(perline_index) = ARC; return os; }
    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_brief(std::basic_ostream<A,B>& os) { os.iword(arcformat_index) = BRIEF; return os; }
    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_full(std::basic_ostream<A,B>& os) { os.iword(arcformat_index) = FULL; return os; }


    void read_training_corpus(std::istream &in,training_corpus &c);
    
    static inline FLOAT_TYPE randomFloat()       // in range [0, 1)
    {
        return rand() * (1.f / (RAND_MAX+1.f));
    }

    bool ownerIn;
    bool ownerOut;
    bool named_states;
    alphabet *in;
    alphabet *out;
    alphabet &in_alph() const 
    {
        return *in;
    }
    alphabet &out_alph() const 
    {
        return *out;
    }
    
    alphabet stateNames;
    unsigned final;	// final state number - initial state always number 0
    typedef dynamic_array<State> StateVector;
    // note: std::vector<State> doesn't work with State::state_adder because copies by value are made during readLegible
    
    StateVector states;
  	 
    //  HashTable<IntKey, int> tieGroup; // IntKey is FSTArc *; value in group number (0 means fixed weight)
    //  WFST(WFST &) {}		// disallow copy constructor - Yaser commented this ow to allow copy constructors
    //  WFST & operator = (WFST &){return *this;} Yaser
    //WFST & operator = (WFST &){std::cerr <<"Unauthorized use of assignemnt operator\n";;return *this;}
    int abort();			// called on a bad read
    int readLegible(istream &,bool alwaysNamed=false);	// returns 0 on failure (bad input)
    int readLegible(const string& str, bool alwaysNamed=false);  
    void writeArc(ostream &os, const FSTArc &a,bool GREEK_EPSILON=false);
    void writeLegible(ostream &);
    void writeLegibleFilename(std::string const& name);
    void writeGraphViz(ostream &); // see http://www.research.att.com/sw/tools/graphviz/
    int numStates() const { return states.size(); }
    bool isFinal(int s) { return s==final; }
    void setPathArc(PathArc *pArc,const FSTArc &a) {
        pArc->in = a.in;
        pArc->out = a.out;
        pArc->destState = a.dest;
        pArc->weight = a.weight;
        pArc->wfst=this;
    }
    std::ostream & printArc(const FSTArc &a,std::ostream &o) {
        PathArc p;
        setPathArc(&p,a);
        return o << p;
    }
    std::ostream & printArc(const FSTArc &a,int source,std::ostream &o) {
        return o << '(' << stateName(source) << " -> " << stateName(a.dest) << ' ' << inLetter(a.in) << " : " << outLetter(a.out) << " / " << a.weight << ")";
    }

    /*void insertPathArc(GraphArc *gArc, List<PathArc>*);  
      void insertShortPath(int source, int dest, List<PathArc> *);*/
    template <class T>
    void insertPathArc(GraphArc *gArc,T &l)
    {
        PathArc pArc;
        setPathArc(&pArc,*gArc->data_as<FSTArc*>());
        *(l++) = pArc;
    }
    template <class T>
    void insertShortPath(GraphState *shortPathTree,int source, int dest, T &l)
    {
        GraphArc *taken;
        for ( int iState = source ; iState != dest; iState = taken->dest ) {
            taken = &shortPathTree[iState].arcs.top();
            insertPathArc(taken,l);
        }
    }
    /*template <>
      void insertPathArc(GraphArc *gArc, List<PathArc>*l) {
      insertPathArc(gArc,insert_iterator<List<PathArc> >(*l,l->erase_begin()));
      }
      template <>
      void insertShortPath(GraphState *shortPathTree,int source, int dest, List<PathArc> *l) {
      insertShortPath(shortPathTree,source,dest,insert_iterator<List<PathArc> >(*l,l->erase_begin()));
      }*/

    static int indexThreshold;
    enum norm_group_by { CONDITIONAL, // all arcs from a state with the same input will add to one
                           JOINT, // all arcs from a state will add to one (thus sum of all paths from start to finish = 1 assuming no dead ends
                           NONE // 
    } ;
    
    struct NormalizeMethod 
    {
        norm_group_by group;
        mean_field_scale scale;
    };
    
    WFST(const WFST &a) { throw std::runtime_error("No copying of WFSTs allowed!"); }
 public:

    
    void index(int dir) {
        for ( int s = 0 ; s < numStates() ; ++s ) {
            states[s].flush();
            states[s].indexBy(dir);
        }
    }
    void indexInput() {
        index(State::input);
    }
    void indexOutput() {
        index(State::output);
    }

    void project(int dir=State::input,bool identity_fsa=false) {
        if (identity_fsa) {
            identity_alphabet_from(dir);
        }
        
        for ( int s = 0 ; s < numStates() ; ++s )
            states[s].project(dir,identity_fsa);
    }
    
    
    void indexFlush() { // index on input symbol or output symbol depending on composition direction
        for ( int s = 0 ; s < numStates() ; ++s ) {
            states[s].flush();
        }
    }
    WFST() { initAlphabet(); named_states=0;}
    //  WFST(const WFST &a): 
    //ownerInOut(1), in(((a.in == 0)? 0:(NEW Alphabet(*a.in)))), out(((a.out == 0)? 0:(NEW Alphabet(*a.out)))), 
    //stateNames(a.stateNames), final(a.final), states(a.states), 

    WFST(istream & istr,bool alwaysNamed=false) {
        initAlphabet();
        if (!this->readLegible(istr,alwaysNamed))
            final = invalid_state;
    }

    WFST(const string &str, bool alwaysNamed){
        initAlphabet();
        if (!this->readLegible(str,alwaysNamed))
            final = invalid_state;
    }

    WFST(const char *buf); // make a simple transducer representing an input sequence
    WFST(const char *buf, int& length,bool permuteNumbers); // make a simple transducer representing an input sequence lattice - Yaser
    WFST(WFST &a, WFST &b, bool namedStates = false, bool preserveGroups = false);	// a composed with b    
    WFST(cascade_parameters &cascade,WFST &a, WFST &b, bool namedStates = false,bool preserveGroups = false);	// a composed with b, but remembering in cascade the identities.  preserveGroups is meaningless since cascade keeps refs to original arcs anyway
    void set_compose(cascade_parameters &cascade,WFST &a, WFST &b, bool namedStates = false, bool preserveGroups = false);
    // resulting WFST has only reference to input/output alphabets - use ownAlphabet()
    // if the original source of the alphabets must be deleted
    void listAlphabet(ostream &out, int output = 0);
    friend ostream & operator << (ostream &,  WFST &); // Yaser 7-20-2000
    // I=PathArc output iterator; returns length of path or -1 on error
    int randomPath(List<PathArc> *l,int max_len=-1) {
        return randomPath(l->back_inserter(), max_len);
    }
    template <class I> int randomPath(I i,int max_len=-1)
    {
        PathArc p;
        int s=0;
        unsigned int len=0;
        unsigned int max=*(unsigned int *)&max_len;
        for (;;) {
            if (s == final)
                return len;
            if  ( len > max || states[s].arcs.isEmpty() )
                return -1;
            // choose random arc:
            Weight arcsum;
            typedef List<FSTArc> LA;
            typedef LA::const_iterator LAit;
            const LA& arcs=states[s].arcs;
            LAit start=arcs.const_begin(),end=arcs.const_end();
            for (LAit li = start; li!=end; ++li) {
                arcsum+=li->weight;
            }
            Weight which_arc = arcsum * randomFloat();
#ifdef DEBUG_RANDOM_GENERATE
            Config::debug() << " chose "<<which_arc<<"/"<<arcsum;
#endif 
            arcsum.setZero();
            for (LAit li = start; li!=end; ++li) {
                if ( (arcsum += li->weight) >= which_arc) {
                    // add arc
                    setPathArc(&p,*li);
                    *i++ = p;
#ifdef DEBUG_RANDOM_GENERATE
                    Config::debug() << " arc="<<*li;
#endif 
                    s=li->dest;
                    ++len;
                    //				if (!(i))
                    //					return -1;
                    goto next_arc;
                }
            }
            throw std::runtime_error("Failed to choose a random arc when generating random paths.  Since we remove useless states, this should never happen");
        next_arc:
            ++len;
        }
    }

    List<List<PathArc> > * randomPaths(int k, int max_len=-1); // gives a list of (up to) k paths
    // random paths to final
    // labels are pointers to names in WFST so do not
    // use the path after the WFST is deleted
    // list is dynamically allocated - delete it
    // yourself when you are done with it

    List<List<PathArc> > *bestPaths(int k); // bestPaths(k) gives a list of the (up to ) k
    // Visitor needs to accept GraphArc (from makeGraph ... (FSTArc *)->data gives WFST FSTArc - see kbest.h for visitor description
    template <class Visitor> void bestPaths(unsigned k,Visitor &v) {
        Graph graph = makeGraph();
        graehl::bestPaths(graph,0,final,k,v);
        freeGraph(graph);
    }
    // best paths to final
    // labels are pointers to names in WFST so do not
    // use the path after the WFST is deleted
    // list is dynamically allocated - delete it
    // yourself when you are done with it
    struct symbol_ids : public List<int> 
    {
        symbol_ids(WFST const& wfst,char const* buf,int output=0,int line=-1) 
        {
            wfst.symbolList(this,buf,output,line);
        }
    };

    static void print_yield(ostream &o,List<PathArc> const &path,bool output=false,bool show_special=false) 
    {
        if (path.empty())
            return;
        graehl::word_spacer sp(' ');
        for (List<PathArc>::const_iterator li=path.const_begin(),end=path.const_end(); li != end; ++li ) {
            WFST const& w=*li->wfst;
            int id=output ? li->out : li->in;
            if (show_special || id!=epsilon_index)
                o << sp << (output ? w.outLetter(id) : w.inLetter(id));
        }
    }

    static void print_training_pair(ostream &o,List<PathArc> const &path,bool show_special=false)
    {
        print_yield(cout,path,false,show_special);
        o << endl;
        print_yield(cout,path,true,show_special);
        o << endl;
    }
    
        
    void symbolList(List<int> *ret,const char *buf, int output=0,int line=-1) const;   
    // takes space-separated symbols and returns a list of symbol numbers in the
    // input or output alphabet
    const char *inLetter(unsigned i) const {
        Assert ( i >= 0 );
        Assert ( i < in->size() );
        return (*in)[i].c_str();
    }
    const char *outLetter(unsigned i) const {
        Assert ( i >= 0 );
        Assert ( i < out->size() );
        return (*out)[i].c_str();
    }
    //NB: uses static (must use or copy before next call) return string buffer if !named_states
    const char *stateName(int i) const {
        Assert ( i >= 0 );
        Assert ( i < numStates() );
        if (named_states)
            return stateNames[i].c_str();
        else
            return static_itoa(i);
    }
    Weight sumOfAllPaths(List<int> &inSeq, List<int> &outSeq);
    // gives sum of weights of all paths from initial->final with the input/output sequence (empties are elided)
    void randomScale() {  // randomly scale weights (of unlocked arcs) before training by (0..1]
        changeEachParameter(scaleRandom);
    }
    void randomSet() { // randomly set weights (of unlocked arcs) on (0..1]
        changeEachParameter(setRandom);
    }
    template <class V>
    void visit_arcs_sourceless(V &v) const
    {
        for (StateVector::const_iterator i=states.begin(),e=states.end();i!=e;++i) {
            i->visit_arcs(v);
        }        
    }
    
    void set_constant_weights(Weight w=Weight::ONE()) 
    {
        changeEachParameter(set_constant_weight(w));
    }
    
    void zero_arcs() 
    {
        set_constant_weights(Weight::ZERO());
    }
    
    void normalize(NormalizeMethod const& method);    
    
    // if weight_is_prior_count, weights before training are prior counts.  smoothFloor counts are also added to all arcs
    // NEW weight = normalize(induced forward/backward counts + weight_is_prior_count*old_weight + smoothFloor).
    // corpus may have examples w/ no derivations removed from it!
        
    Weight train(training_corpus & corpus,NormalizeMethod const& method,bool weight_is_prior_count, Weight smoothFloor,Weight converge_arc_delta, Weight converge_perplexity_ratio, int maxTrainIter,FLOAT_TYPE learning_rate_growth_factor=1.,int ran_restarts=0,unsigned cache_derivations_level=0);
    Weight train(cascade_parameters &cascade,training_corpus & corpus,NormalizeMethod const& method,bool weight_is_prior_count, Weight smoothFloor,Weight converge_arc_delta, Weight converge_perplexity_ratio, int maxTrainIter,FLOAT_TYPE learning_rate_growth_factor=1.,int ran_restarts=0,unsigned cache_derivations_level=0); // set weights in original composed transducers (the transducers that were composed w/ the given cascade object and must still be valid for updating arcs/normalizing)
    
    // returns per-example perplexity achieved
    enum { cache_nothing=0,cache_forward=1,cache_forward_backward=2 
    }; // cache_derivations_level param

    
//    Weight trainFinish();
    // stop if greatest change in arc weight, or per-example perplexity is less than criteria, or after set number of iterations.  

    void invert();		// switch input letters for output letters
    void reduce();		// eliminate all states not along a path from
                                // initial state to final state
    void consolidateArcs(bool sum=true,bool clamp=true);	// combine identical arcs, with combined weight = sum, or just max if sum=false.  sum clamped to max of 1 if clamp=true
    void pruneArcs(Weight thresh);	// remove all arcs with weight < thresh
    enum {UNLIMITED=-1};
    void prunePaths(int max_states=UNLIMITED,Weight keep_paths_within_ratio=Weight::INF()); 
    // throw out rank states by the weight of the best path through them, keeping only max_states of them (or all of them, if max_states<0), after removing states and arcs that do not lie on any path of weight less than (best_path/keep_paths_within_ratio)
  
  
    void assignWeights(const WFST &weightSource); // for arcs in this transducer with the same group number as an arc in weightSource, assign the weight of the arc in weightSource.  if no arc having same group number in weightSource is found, remove the arc from *this
    void numberArcsFrom(int labelStart); // sequentially number each arc (placing it into that group) starting at labelStart - labelStart must be >= 1
    void lockArcs();		// put all arcs in group 0 (weights are locked)
    //  void unTieGroups() { tieGroup.~HashTable(); PLACEMENT_NEW (&tieGroup) HashTable<IntKey, int>; }
    void unTieGroups();
  
  
    int generate(int *inSeq, int *outSeq, int minArcs, int maxArcs);
    BOOST_STATIC_CONSTANT(unsigned,invalid_state=(unsigned)-1);
    int valid() const { return ( final != invalid_state ); }
    unsigned int size() const { if ( !valid() ) return 0; else return numStates(); }
    int numArcs() const {
        int a = 0;
        for (int i = 0 ; i < numStates() ; ++i )
            a += states[i].size;
        return a;
    }
    Weight numNoCyclePaths() {
        if ( !valid() ) return Weight();
        Weight *nPaths = NEW Weight[numStates()];
        Graph g = makeGraph();
        countNoCyclePaths(g, nPaths, 0);
        delete[] g.states;
        Weight ret = nPaths[final];
        delete[] nPaths;
        return ret;
    }
    static void setIndexThreshold(int t) {
        if ( t < 0 )
            WFST::indexThreshold = 0;
        else
            WFST::indexThreshold = t; 
    }
    Graph makeGraph(); // weights = -log, so path length is sum and best path 
    // is the shortest; GraphArc::data is a pointer 
    // to the FSTArc it corresponds to in the WFST
    Graph makeEGraph(); // same as makeGraph, but restricted to *e* / *e* arcs
    void ownAlphabet() {
        ownInAlphabet();
        ownOutAlphabet();
    }
    void stealInAlphabet(WFST &from) {
        if (from.ownerIn && from.in == in) {
            from.ownerIn=0;
            ownerIn=1;
        } else
            ownInAlphabet();
    }
    void stealOutAlphabet(WFST &from) {
        if (from.ownerOut && from.out == out) {
            from.ownerOut=0;
            ownerOut=1;
        } else
            ownOutAlphabet();
    }
    void ownInAlphabet() {
        if ( !ownerIn ) {
            in = NEW alphabet(*in);
            ownerIn = 1;
        }
    }
    void ownOutAlphabet() {
        if ( !ownerOut ) {
            out = NEW alphabet(*out);
            ownerOut = 1;
        }
    }
    void unNameStates() {
        if (named_states) {
            stateNames.~Alphabet();
            named_states=false;
            PLACEMENT_NEW (&stateNames) alphabet();
        }
    }

    void raisePower(double exponent=1.0) 
    {
        for ( int s = 0 ; s < numStates() ; ++s )
            states[s].raisePower(exponent);
    }
    
    // returns id of newly added empty state
    unsigned add_state(char const* name="NEWSTATE") 
    {
        unsigned r=states.size();
        states.push_back();
        if (named_states) {
            unsigned equals_r=stateNames.add_make_unique(name);
            Assert(equals_r==r);
        }
        return r;
    }
    
    void clear() {
        final = invalid_state;        
        unNameStates();
        states.clear();
        destroy();
    }
    ~WFST() {
        destroy();
    }
    typedef void (*WeightChanger)(Weight *w);
    static void setRandom(Weight *w) {
        Weight random;
        random.setRandomFraction();
        *w = random;
    }
    static void scaleRandom(Weight *w) {
        Weight random;
        random.setRandomFraction();
        *w *= random;
    }
    struct set_constant_weight 
    {
        Weight c;
        set_constant_weight(Weight c) : c(c) {}
        set_constant_weight(set_constant_weight const& o) : c(o.c) {}
        void operator()(Weight *w) const
        {
            *w=c;
        }
    };
    
        
    template <class F> void readEachParameter(F f) {
        HashTable<IntKey, bool> seenGroups;
        for ( int s = 0 ; s < numStates() ; ++s )
            for ( List<FSTArc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )  {
                int group=a->groupId;
                if (isNormal(group) || FSTArc::tied(group) && seenGroups.insert(HashTable<IntKey, bool>::value_type(group,true)).second)
                    f((const Weight *)&(a->weight));
            }
    }
    // respects locked/tied arcs.  f(&weight) changes weight once per group or ungrouped arc, and never for locked
    template <class F> void changeEachParameter(F f) {
        typedef HashTable<IntKey, Weight> HT;
            
        HT tiedWeights;
        for ( int s = 0 ; s < numStates() ; ++s )
            for ( List<FSTArc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )  {
                int group=a->groupId;
                if (isLocked(group))
                    return;
                if (isNormal(group))
                    f(&(a->weight));
                else if (FSTArc::tied(group)) { // assumption: all the weights for the tie group are the same (they should be, after normalization at least)
//#define OLD_EACH_PARAM
                    //hash_traits<HT>::insert_return_type
                    HT::insert_return_type it;
                    if ((it=tiedWeights.insert(HT::value_type(group,0))).second) {
                        f(&(a->weight));
                        it.first->second = a->weight;
                    } else
                        a->weight = it.first->second;
                }
            }
    }
    void removeMarkedStates(bool marked[]);  // remove states and all arcs to
    // states marked true
    BOOST_STATIC_CONSTANT(int,no_group=FSTArc::no_group);
    BOOST_STATIC_CONSTANT(int,locked_group=FSTArc::locked_group);
    static inline bool isNormal(int groupId) {
        return FSTArc::normal(groupId);
    }
    static inline bool isLocked(int groupId) {
        return FSTArc::locked(groupId);
    }
    static inline bool isTied(int groupId) {
        return FSTArc::tied(groupId);
    }
    static inline bool isTiedOrLocked(int groupId) {
        return !FSTArc::normal(groupId);
    }

    // v(unsigned source_state,FSTArc &arc)
    template <class V>
    void visit_arcs(V & v) 
    {
//        unsigned arcno=0;
        for ( unsigned s = 0,e=numStates() ; s < e ; ++s )
            states[s].visit_arcs(s,v);
    }
    

    
 private:
        

    //	lastChange = train_maximize(method);
    // counts must have been filled in (happens in trainFinish) so not useful to public
    Weight train_maximize(NormalizeMethod const& method,FLOAT_TYPE delta_scale=1); // normalize then exaggerate (then normalize again), returning maximum change
    
    void destroy()  // just in case we're sloppy, this is idempotent.  note: the actual destructor may not be - std::vector, etc.
    {
        deleteAlphabet();
    }
    
    void invalidate() {		// make into empty/invalid transducer
        clear();
    }
};

ostream & operator << (ostream &o, WFST &w);

ostream & operator << (std::ostream &o, List<PathArc> &l);


}

#endif

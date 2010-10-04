
#ifndef STATS_BUILDER_H
#define STATS_BUILDER_H

#include <globals.h>
#include <superstl.h>

#include <yaml/yaml.h>

#define STATS_SIZE 1024*10

class StatObjBase;
class Stats;

inline static YAML::Emitter& operator << (YAML::Emitter& out, const W64 value)
{
    stringbuf buf;
    buf << value;
    out << buf;
    return out;
}

/**
 * @brief Base class for all classes that has Stats counters
 *
 * To add stats to global statsistics mechanism, inherit this class.
 */
class Statable {
    private:
        dynarray<Statable*> childNodes;
        dynarray<StatObjBase*> leafs;
        Statable *parent;
        stringbuf name;

        Stats *default_stats;

    public:
        /**
         * @brief Constructor for Statable without any parent
         *
         * @param name Name of the Statable class, used in YAML key
         * @param is_root Don't use it, only for internal purpose
         */
        Statable(const char *name, bool is_root = false);

        /**
         * @brief Constructor for Statable without any parent
         *
         * @param name Name of the Statable class, used in YAML key
         * @param is_root Don't use it, only for internal purpose
         */
        Statable(stringbuf &str, bool is_root = false);

        /**
         * @brief Contructor for Statable with parent
         *
         * @param name Name of the Statable class, used in YAML key
         * @param parent Parent Statable object
         *
         * By providing parent class, we build a hierarhcy of Statable objects
         * and use it to print hierarhical Statistics.
         */
        Statable(const char *name, Statable *parent=NULL);

        /**
         * @brief Constructor for Statable with parent
         *
         * @param str Name of the Statable class, used in YAML Key
         * @param parent Parent Statable object
         */
        Statable(stringbuf &str, Statable *parent=NULL);

        /**
         * @brief Add a child Statable node into this object
         *
         * @param child Child Statable object to add
         */
        void add_child_node(Statable *child)
        {
            childNodes.push(child);
        }

        /**
         * @brief Add a child StatObjBase node into this object
         *
         * @param edge A StatObjBase which contains information of Stats
         */
        void add_leaf(StatObjBase *edge)
        {
            leafs.push(edge);
        }

        /**
         * @brief get default Stats*
         *
         * @return
         */
        Stats* get_default_stats()
        {
            return default_stats;
        }

        /**
         * @brief Set default Stats* for this Statable and all its Child
         *
         * @param stats
         */
        void set_default_stats(Stats *stats);

        /**
         * @brief Dump string representation of  Statable and it childs
         *
         * @param os
         * @param stats Stats object from which get stats data
         *
         * @return
         */
        ostream& dump(ostream &os, Stats *stats);

        /**
         * @brief Dump YAML representation of Statable and its childs
         *
         * @param out
         * @param stats Stats object from which get stats data
         *
         * @return
         */
        YAML::Emitter& dump(YAML::Emitter &out, Stats *stats);

        void add_stats(Stats& dest_stats, Stats& src_stats);
};

/**
 * @brief Builder interface to for Stats object
 *
 * This class provides interface to build a Stats object and also provides and
 * interface to map StatObjBase objects to Stats memory.
 *
 * This is a 'singleton' class, so get the object using 'get()' function.
 */
class StatsBuilder {
    private:
        static StatsBuilder _builder;
        Statable *rootNode;
        W64 stat_offset;

        StatsBuilder()
        {
            rootNode = new Statable("", true);
            stat_offset = 0;
        }

        ~StatsBuilder()
        {
            delete rootNode;
        }

    public:
        /**
         * @brief Get the only StatsBuilder object reference
         *
         * @return
         */
        static StatsBuilder& get()
        {
            return _builder;
        }

        /**
         * @brief Add a Statble to the root of the Stats tree
         *
         * @param statabl
         */
        void add_to_root(Statable *statable)
        {
            assert(statable);
            assert(rootNode);
            rootNode->add_child_node(statable);
        }

        /**
         * @brief Get the offset for given StatObjBase class
         *
         * @param size Size of the memory to be allocted
         *
         * @return Offset value
         */
        W64 get_offset(int size)
        {
            W64 ret_val = stat_offset;
            stat_offset += size;
            assert(stat_offset < STATS_SIZE);
            return ret_val;
        }

        /**
         * @brief Get a new Stats object
         *
         * @return new Stats*
         */
        Stats* get_new_stats();

        /**
         * @brief Delte Stats object
         *
         * @param stats
         */
        void destroy_stats(Stats *stats);

        /**
         * @brief Dump whole Stats tree to ostream
         *
         * @param stats Use given Stats* for values
         * @param os ostream object to dump string
         *
         * @return
         */
        ostream& dump(Stats *stats, ostream &os) const;

        /**
         * @brief Dump Stats tree in YAML format
         *
         * @param stats Use given Stats* for values
         * @param out YAML::Emitter object to dump YAML represetation
         *
         * @return
         */
        YAML::Emitter& dump(Stats *stats, YAML::Emitter &out) const;

        void add_stats(Stats& dest_stats, Stats& src_stats)
        {
            rootNode->add_stats(dest_stats, src_stats);
        }
};

/**
 * @brief Statistics class that stores all the statistics counters
 *
 * Stats basically contains a fix memory which is used by all StatObjBase
 * classes to store their variables. Users are not allowed to directly create
 * an object of Stats, they must used StatsBuilder::get_new_stats() function
 * to get one.
 */
class Stats {
    private:
        W8 *mem;

        Stats()
        {
            mem = NULL;
        }

    public:
        friend class StatsBuilder;

        W64 base()
        {
            return (W64)mem;
        }

        Stats& operator+=(Stats& rhs_stats)
        {
            (StatsBuilder::get()).add_stats(*this, rhs_stats);
            return *this;
        }
};

/**
 * @brief Base class for all Statistics container classes
 */
class StatObjBase {
    protected:
        Stats *default_stats;
        Statable *parent;
        stringbuf name;

    public:
        StatObjBase(const char *name, Statable *parent)
            : parent(parent)
        {
            this->name = name;
            default_stats = parent->get_default_stats();
            parent->add_leaf(this);
        }

        virtual void set_default_stats(Stats *stats);

        virtual ostream& dump(ostream& os, Stats *stats) const = 0;
        virtual YAML::Emitter& dump(YAML::Emitter& out,
                Stats *stats) const = 0;

        virtual void add_stats(Stats& dest_stats, Stats& src_stats) = 0;
};

/**
 * @brief Create a Stat object of type T
 *
 * This class povides an easy interface to create basic statistics counters for
 * the simulator. It also provides easy way to generate YAML representation of
 * the the object for final Stats dump.
 */
template<typename T>
class StatObj : public StatObjBase {
    private:
        W64 offset;

        T *default_var;

        inline void set_default_var_ptr()
        {
            if(default_stats) {
                default_var = (T*)(default_stats->base() + offset);
            } else {
                default_var = NULL;
            }
        }

    public:
        /**
         * @brief Default constructor for StatObj
         *
         * @param name Name of that StatObj used in YAML Key map
         * @param parent Statable* which contains this stats object
         */
        StatObj(const char *name, Statable *parent)
            : StatObjBase(name, parent)
        {
            StatsBuilder &builder = StatsBuilder::get();

            offset = builder.get_offset(sizeof(T));

            set_default_var_ptr();
        }

        /**
         * @brief Set the default Stats*
         *
         * @param stats A pointer to Stats structure
         *
         * This function will set the default Stats* to use for all future
         * operations like ++, += etc. untill its changed.
         */
        void set_default_stats(Stats *stats)
        {
            StatObjBase::set_default_stats(stats);
            set_default_var_ptr();
        }

        /**
         * @brief ++ operator - like a++
         *
         * @param dummy
         *
         * @return object of type T with updated value
         */
        inline T operator++(int dummy)
        {
            assert(default_var);
            T ret = (*default_var)++;
            return ret;
        }

        /**
         * @brief ++ operator - like ++a
         *
         * @return object of type T with updated value
         */
        inline T operator++()
        {
            assert(default_var);
            (*default_var)++;
            return (*default_var);
        }

        /**
         * @brief -- opeartor - like a--
         *
         * @param dummy
         *
         * @return object of type T with update value
         */
        inline T operator--(int dummy) {
            assert(default_var);
            T ret = (*default_var)--;
            return ret;
        }

        /**
         * @brief -- opeartor - like --a
         *
         * @return object of type T with update value
         */
        inline T operator--() {
            assert(default_var);
            (*default_var)--;
            return (*default_var);
        }

        /**
         * @brief + operator
         *
         * @param b value to be added
         *
         * @return object of type T with new value
         */
        inline T operator +(const T &b) const {
            assert(default_var);
            T ret = (*default_var) + b;
            return ret;
        }

        /**
         * @brief + operator with StatObj
         *
         * @param statObj
         *
         * @return object of type T with new value
         */
        inline T operator +(const StatObj<T> &statObj) const {
            assert(default_var);
            assert(statObj.default_var);
            T ret = (*default_var) + (*statObj.default_var);
            return ret;
        }

        /**
         * @brief - operator
         *
         * @param b value to remove
         *
         * @return object of type T with new value
         */
        inline T operator -(const T &b) const {
            assert(default_var);
            T ret = (*default_var) - b;
            return ret;
        }

        /**
         * @brief - operator with StatObj
         *
         * @param statObj
         *
         * @return object of type T with new value
         */
        inline T operator -(const StatObj<T> &statObj) const {
            assert(default_var);
            assert(statObj.default_var);
            T ret = (*default_var) - (*statObj.default_var);
            return ret;
        }

        /**
         * @brief * operator
         *
         * @param b value to multiply
         *
         * @return object of type T with new value
         */
        inline T operator *(const T &b) const {
            assert(default_var);
            T ret = (*default_var) * b;
            return ret;
        }

        /**
         * @brief * operator with StatObj
         *
         * @param statObj
         *
         * @return object of type T with new value
         */
        inline T operator *(const StatObj<T> &statObj) const {
            assert(default_var);
            assert(statObj.default_var);
            T ret = (*default_var) * (*statObj.default_var);
            return ret;
        }

        /**
         * @brief / operator
         *
         * @param b value to divide
         *
         * @return object of type T with new value
         */
        inline T operator /(const T &b) const {
            assert(default_var);
            T ret = (*default_var) / b;
            return ret;
        }

        /**
         * @brief / operator with StatObj
         *
         * @param statObj
         *
         * @return object of type T with new value
         */
        inline T operator /(const StatObj<T> &statObj) const {
            assert(default_var);
            assert(statObj.default_var);
            T ret = (*default_var) / (*statObj.default_var);
            return ret;
        }

        /**
         * @brief () operator to use given Stats* instead of default
         *
         * @param stats Stats* to use instead of default Stats*
         *
         * @return reference of type T in given Stats
         *
         * This function can be used when you don't want to operate on the
         * default stats of the object and specify other Stats* without
         * changing the default_stats in the object.
         *
         * For example, use this function like below:
         *      cache.read.hit(kernel_stats)++
         * This will increase the 'cache.read.hit' counter in kerenel_stats
         * instead of 'default_stats' of 'cache.read.hit'.
         */
        inline T& operator()(Stats *stats) const
        {
            return *(T*)(stats->base() + offset);
        }

        /**
         * @brief Dump a string representation to ostream
         *
         * @param os ostream& where string will be added
         * @param stats Stats object from which get stats data
         *
         * @return updated ostream object
         */
        ostream& dump(ostream& os, Stats *stats) const
        {
            T var = (*this)(stats);

            os << name << ":" << var << "\n";

            return os;
        }

        /**
         * @brief Dump YAML representation of this object
         *
         * @param out YAML::Emitter object
         * @param stats Stats object from which get stats data
         *
         * @return
         */
        YAML::Emitter& dump(YAML::Emitter &out, Stats *stats) const
        {
            assert(default_var);
            T var = (*this)(stats);

            out << YAML::Key << (char *)name;
            out << YAML::Value << var;

            return out;
        }

        void add_stats(Stats& dest_stats, Stats& src_stats)
        {
            T& dest_var = (*this)(&dest_stats);
            dest_var += (*this)(&src_stats);
        }
};

/**
 * @brief Create a Stat object which is array of T[size]
 *
 * This class provides easy interface to create an array of statistic counters
 */
template<typename T, int size>
class StatArray : public StatObjBase {

    private:
        W64 offset;
        T* default_var;

        inline void set_default_var_ptr()
        {
            if(default_stats) {
                default_var = (T*)(default_stats->base() + offset);
            } else {
                default_var = NULL;
            }
        }

    public:

        typedef T BaseArr[size];

        /**
         * @brief Default constructor
         *
         * @param keyName Name of the stat array
         */
        StatArray(const char *name, Statable *parent)
            : StatObjBase(name, parent)
        {
            StatsBuilder &builder = StatsBuilder::get();

            offset = builder.get_offset(sizeof(T) * size);

            set_default_var_ptr();
        }

        /**
         * @brief Set the default Stats*
         *
         * @param stats A pointer to Stats structure
         *
         * This function will set the default Stats* to use for all future
         * operations like ++, += etc. untill its changed.
         */
        void set_default_stats(Stats *stats)
        {
            StatObjBase::set_default_stats(stats);
            set_default_var_ptr();
        }

        /**
         * @brief [] operator to access one element of array
         *
         * @param index index of the element to access from array
         *
         * @return reference to the requested element
         */
        inline T& operator[](const int index)
        {
            assert(index < size);
            assert(default_var);

            return *(T*)(default_var + (index * sizeof(T)));
        }

        /**
         * @brief () operator to use given Stats* instead of default
         *
         * @param stats Stats* to use instead of default Stats*
         *
         * @return reference of array of type T in given Stats
         *
         */
        BaseArr& operator()(Stats *stats) const
        {
            return *(BaseArr*)(stats->base() + offset);
        }

        /**
         * @brief dump string representation of StatArray
         *
         * @param os dump into this ostream object
         * @param stats Stats* from which to get array values
         *
         * @return
         */
        ostream& dump(ostream &os, Stats *stats) const
        {
            os << name << ": ";
            BaseArr& arr = (*this)(stats);
            foreach(i, size) {
                os << arr[i] << " ";
            }
            os << "\n";

            return os;
        }

        /**
         * @brief dump YAML representation of StatArray
         *
         * @param out dump into this object
         * @param stats Stats* from which to get array values
         *
         * @return
         */
        YAML::Emitter& dump(YAML::Emitter &out, Stats *stats) const
        {
            out << YAML::Key << (char *)name;
            out << YAML::Value;

            out << YAML::Flow;
            out << YAML::BeginSeq;

            BaseArr& arr = (*this)(stats);
            foreach(i, size) {
                out << arr[i];
            }

            out << YAML::EndSeq;
            out << YAML::Block;

            return out;
        }

        void add_stats(Stats& dest_stats, Stats& src_stats)
        {
            BaseArr& dest_arr = (*this)(&dest_stats);
            BaseArr& src_arr = (*this)(&src_stats);
            foreach(i, size) {
                dest_arr[i] += src_arr[i];
            }
        }
};

#endif // STATS_BUILDER_H

/*
   Implement a simple object factory without RTTI

   To use:  Base class contains a singleton Factory object declared as QNFactory< baseType > and a static method to return a reference or pointer to it

             class Base { ....
                            public Factory<Base> *getFactory()
                          ...}

            Descendant classes must have a default constructor and "register" their type with the factory by calling Base::registerType( String name ):

            Class Child : Base {...
                            public Child() {}
                            public static registerType() { Base::getFactory()->registerType("Child"); }
                          ...}

           To create a dynamic instanc of a child class, call the create method with a String representing the registered type:

           Child *newChild = Base::getFactory()->create("Child");
                          
*/
template <typename T>
class QNFactory
{
public:

    template <typename TDerived>
    void registerType(String name)
    {
        static_assert(std::is_base_of<T, TDerived>::value, "Factory::registerType doesn't accept this type because doesn't derive from base class");
        TypeMap newType;
        newType.name = name;
        newType.ctor = &createFunc<TDerived>;
        _createFuncs.push_back( newType );
    }

    T* create(String name) {
        T* result;
        boolean fnd = false;
        for (auto i = _createFuncs.begin(); i != _createFuncs.end(); ++i) {
          if ((*i).name == name) {  fnd=true; result = (*i).ctor(); break; }
        }
        if (fnd) {
            return result;
        }
        return nullptr;
    }

private:
    template <typename TDerived>
    static T* createFunc()
    {
        return new TDerived();
    }

    typedef T* (*PCreateFunc)();
    class TypeMap { public:  String name; PCreateFunc ctor; };
    std::vector<TypeMap> _createFuncs = std::vector<TypeMap>();
};

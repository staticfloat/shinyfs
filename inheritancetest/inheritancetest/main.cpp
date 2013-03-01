#include <stdio.h>

// forward declare classes
class A;
class B;
class AA;
class BB;

// parents
class A
{
    virtual B* getB_impl();
public:
    B* getB() { return getB_impl(); }
};
class B
{
    virtual A* getA_impl();
public:
    A* getA() { return getA_impl(); }
};

// kids
class AA : public A
{
    virtual BB* getBB_impl();
    B* getB_impl();
public:
    BB* getB() { return getBB_impl(); }
};
class BB : public B
{
    virtual AA* getAA_impl();
    A* getA_impl();
public:
    AA* getA() { return getAA_impl(); }
};

// implement parents
B* A::getB_impl() { return new B; }
A* B::getA_impl() { return new A; }

// implement kids
B* AA::getB_impl() { return getBB_impl(); }
BB* AA::getBB_impl() { return new BB; }
A* BB::getA_impl() { return getAA_impl(); }
AA* BB::getAA_impl() { return new AA; }

int main(int argc, const char * argv[])
{
    // check
    A a; B b;
    A* pa; B* pb;
    AA aa; BB bb;
    AA* paa; BB* pbb;
    
    pa = b.getA();
    pb = a.getB();
    
    pa = bb.getA();
    pb = aa.getB();
    
    paa = bb.getA();
    pbb = aa.getB();
    return 0;
}


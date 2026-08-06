/* func_02037d94 is a compiler-generated this-adjusting virtual-destructor
 * thunk (CodeWarrior _ZThn16_...D1Ev form): ldr ip,[pc,#4]; add r0,r0,ip; b D1
 * The thunk adjusts `this` by -16 (the size of the primary base subobject) then
 * tail-branches to the complete object destructor (SphereClsn::~SphereClsn @0x2037cb0).
 * It cannot be expressed as plain C; the byte-identical thunk is emitted by
 * compiling the C++ multiple-inheritance class below (secondary base at offset 16).
 * Matched symbol: _ZThn16_N7DerivedD1Ev  (== func_02037d94).
 *
 * compile as C++:  mwccarm -lang=c++ -O4,p -enum int -char signed -interworking -proc arm946e -gccext,on
 */
struct Base1 { int a; int b; int c; virtual ~Base1(); virtual void f1(); };
struct Base2 { int b; virtual ~Base2(); virtual void g1(); };
struct Derived : Base1, Base2 { virtual ~Derived(); };
Derived::~Derived() {}

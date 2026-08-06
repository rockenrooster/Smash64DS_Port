/* func_020375c0 is a compiler-generated this-adjusting virtual-destructor
 * thunk (CodeWarrior _ZThn16_N...D1Ev form): ldr ip,[pc,#4]; add r0,r0,ip; b D1
 * Adjusts this by -16 and tail-branches to RaycastGround::~RaycastGround.
 * compile as C++:  mwccarm -O4,p -enum int -char signed -interworking -proc arm946e -gccext,on
 */
struct RaycastGround { int a; int b; int c; virtual ~RaycastGround(); virtual void f1(); };
struct Base2 { int x; virtual ~Base2(); virtual void g1(); };
struct Derived : RaycastGround, Base2 { virtual ~Derived(); };
Derived::~Derived() {}

//cpp
struct State {};
extern State data_ov002_021105a4;
extern State data_ov002_0210ffec;

struct Player {
    int GetHealth();
    int IsInAir();
    int IsState(State &);
    int ChangeState(State &);
};

extern "C" int func_ov002_020d0d2c(void *c);
extern "C" int func_ov002_020d0178(Player *c, int a, int b);

extern "C" int func_ov002_020d0580(Player *p)
{
    int b354, b358;

    if (p->GetHealth() != 0 &&
        *(unsigned char *)((char *)p + 0x709) == 0 &&
        *(unsigned char *)((char *)p + 0x706) == 0)
    {
        b354 = (*(int *)((char *)p + 0x354) != 0);
        if (b354 == 0 &&
            *(int *)((char *)p + 0x37c) == 0)
        {
            b358 = (*(int *)((char *)p + 0x358) != 0);
            if (b358 == 0 &&
                *(unsigned char *)((char *)p + 0x708) == 0 &&
                *(unsigned char *)((char *)p + 0x6fd) == 0 &&
                p->IsInAir() != 0 &&
                func_ov002_020d0d2c(p) == 0)
            {
                goto cont;
            }
        }
    }
    return 0;

cont:
    if (p->IsState(data_ov002_021105a4)) {
        if (*(unsigned char *)((char *)p + 0x6e3) != 0)
            return 0;
    }
    if (*(unsigned char *)((char *)p + 0x6de) == 0 ||
        *(int *)((char *)p + 0xa8) > 0)
        return 0;
    if (func_ov002_020d0178(p, *(int *)((char *)p + 0x60), 0) == 0)
        return 0;
    *(unsigned char *)((char *)p + 0x6e3) = 1;
    p->ChangeState(data_ov002_0210ffec);
    return 1;
}

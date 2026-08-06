extern volatile unsigned short data_ov002_0210c208[];

void _ZN3HUD15CalculateDigitsEt(char* thiz, unsigned short value)
{
    int flag = 0;
    int i;
    for (i = 0; i < 3; i++) {
        int digit = value / data_ov002_0210c208[i];
        if (digit == 0 && flag == 0 && i != 2) {
            (thiz + i)[0x74] = -1;
        } else {
            (thiz + i)[0x74] = digit;
            flag = 1;
        }
        value = value % data_ov002_0210c208[i];
    }
}

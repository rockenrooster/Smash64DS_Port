from math import isqrt


def ceil_sqrt_ratio(num, m):
    # smallest t with t*t*m >= num
    t = isqrt(num // m)
    while t * t * m < num:
        t += 1
    while t > 0 and (t - 1) * (t - 1) * m >= num:
        t -= 1
    return t


even = [ceil_sqrt_ratio(1 << 48, m) for m in range(32, 64)]
odd = [ceil_sqrt_ratio(1 << 49, m) for m in range(32, 64)]


def fmt(name, vals):
    out = [f'static const uint32_t {name}[32] = {{']
    for i in range(0, 32, 4):
        out.append('    ' + ', '.join(f'{v}u' for v in vals[i:i + 4]) + ',')
    out.append('};')
    return '\n'.join(out)


print(fmt('sNdsP2HbInvSqrtEven', even))
print(fmt('sNdsP2HbInvSqrtOdd', odd))

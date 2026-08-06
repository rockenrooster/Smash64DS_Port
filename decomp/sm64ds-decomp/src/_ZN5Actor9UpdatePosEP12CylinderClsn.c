/* Actor::UpdatePos — applies motion direction + vertical acceleration.
 * Calls UpdatePosWithHorzSpeedAndAng first, then UpdatePosWithOnlySpeed.
 *
 * Callees (symbols/verified.tsv):
 *   0x02010c5c = Actor::UpdatePosWithHorzSpeedAndAng()
 *     _ZN5Actor28UpdatePosWithHorzSpeedAndAngEv
 *   0x02010d40 = Actor::UpdatePosWithOnlySpeed(CylinderClsn*)
 *     _ZN5Actor22UpdatePosWithOnlySpeedEP12CylinderClsn
 */

struct CylinderClsn;
struct Actor;

extern void _ZN5Actor28UpdatePosWithHorzSpeedAndAngEv(struct Actor* self);
extern void _ZN5Actor22UpdatePosWithOnlySpeedEP12CylinderClsn(struct Actor* self, struct CylinderClsn* clsn);

void _ZN5Actor9UpdatePosEP12CylinderClsn(struct Actor* this_, struct CylinderClsn* clsn)
{
    _ZN5Actor28UpdatePosWithHorzSpeedAndAngEv(this_);
    _ZN5Actor22UpdatePosWithOnlySpeedEP12CylinderClsn(this_, clsn);
}

"""NOQ SOS kutusu — PETG, üç parça: taban (yığın yukarıdan girer), kapak (geçmeli), düğme pimi.

Ölçüler (kumpasla, Kapton'a sarılı yığın): yığın 35 × 22.7 × 10, düğme tepesi tabandan 13.7, USB-C soketinin üstü tabandan 9.5.
Düğme merkezi USB-C ucundan 12.5, enine ortada. USB-C enine ortada (yuva 15 mm geniş, ±1.3 mm tolerans).
Pilin ya da yerleşimin farklıysa aşağıdaki ölçüleri değiştirip yeniden üret: python3 sos_case.py <çıktı_klasörü>
Eksenler: X boy (−X = USB-C ucu), Y en, Z yükseklik (0 = alt yüz, göğse gelen taraf).
Baskı: taban dik, kapak ters (ayrım yüzü tablada), pim yaka altta — hiçbirinde destek yok, en uzun köprü 7.6 mm.
"""
import math
import sys

import cadquery as cq

# ---- yığın ve paylar
L, B, H, HB = 35.0, 22.7, 10.0, 13.7
BX, BY = 12.5, 0.0               # düğme merkezi: USB ucundan, enine merkezden
USB_UST = 9.5                    # soket üstü, yığın tabanından
PAY = 0.3
LC, BC, RC = L + 2 * PAY, B + 2 * PAY, 3.0
FL = 1.6                         # taban kalınlığı
ZS = FL + H + PAY                # ayrım yüzü = boşluk tavanı 11.9
XB, YB = -LC / 2 + PAY + BX, BY
ZBT = FL + HB                    # düğme tepesi 15.3
USB_Z = FL + USB_UST - 1.6       # soket merkezi (soket 3.2 yüksek)
USB_G, USB_Y = 15.0, 7.2         # fiş gövdesi en çok 12.35 × 6.5

# ---- geçme: tabanın iç dili kapaktaki oluğa girer, uzun kenarlarda tık
DIL_H, DIL_T, DIL_B = 1.5, 1.1, 0.05
CEKIRDEK = 0.6                   # kapak çekirdeği dilden geri çekik: dil içe esneyebilsin
TIK = 0.25                       # tık çıkıntısı

# ---- pim
YAKA_R, YAKA_H, MIL_R, DELIK_R, CEP_R = 3.5, 0.8, 2.0, 2.2, 3.8
GOVDE_R, GOVDE_UST = 4.6, ZBT - 0.5   # 6 × 6 düğme gövdesi hangi açıyla durursa dursun sığsın (köşegen yarısı 4.24)
CEP_UST = ZBT + YAKA_H + 0.2     # pim 0.2 boşlukla yukarı oynar

# ---- dış biçim: süper elips kesitler, üstte süper elips yay
N = 3.8
A, BB = 22.2, 15.4
Z0, HT, P = 12.0, 5.6, 2.4
_ust = [1.0, 0.99, 0.97, 0.94, 0.90, 0.85, 0.78, 0.70, 0.60, 0.48, 0.35]
PROFIL = [(0.0, 0.90), (0.6, 0.955), (1.5, 0.99), (2.8, 1.0), (4.6, 1.0), (6.4, 1.0), (8.2, 1.0), (10.0, 1.0)] + [
    (round(Z0 + HT * (1 - k ** P) ** (1 / P), 3), k) for k in _ust]


def ust_z(x, y):
    sk = min(((abs(x) / A) ** N + (abs(y) / BB) ** N) ** (1 / N), 1.0)
    return Z0 + HT * (1 - sk ** P) ** (1 / P)


def superelips(a, b, n, k=160):
    pts = []
    for i in range(k):
        t = 2 * math.pi * i / k
        c, s_ = math.cos(t), math.sin(t)
        pts.append((a * math.copysign(abs(c) ** (2 / n), c), b * math.copysign(abs(s_) ** (2 / n), s_)))
    return pts


def kutu_r(lx, ly, z0, z1, r, cx=0.0, cy=0.0):
    return cq.Workplane("XY").box(lx, ly, z1 - z0, centered=(True, True, False)).edges("|Z").fillet(r).translate((cx, cy, z0))


def ofset(d, z0, z1):
    """Boşluk dikdörtgeninin d kadar dışa (eksi: içe) kaydırılmışı."""
    return kutu_r(LC + 2 * d, BC + 2 * d, z0, z1, RC + d)


def silindir(r, z0, z1, x=0.0, y=0.0):
    return cq.Workplane("XY").center(x, y).circle(r).extrude(z1 - z0).translate((0, 0, z0))


def dis():
    teller = []
    for z, k in PROFIL:
        pts = [cq.Vector(x, y, z) for x, y in superelips(A * k, BB * k, N)]
        teller.append(cq.Wire.assembleEdges([cq.Edge.makeSpline(pts, periodic=True)]))
    g = cq.Workplane("XY").add(cq.Solid.makeLoft(teller, ruled=False))
    g = g.union(cq.Workplane("XY").add(cq.Face.makeFromWires(teller[-1])).extrude(-0.3))
    # düğme merceği: pimin çevresinde 0.5 kabaran halka, karanlıkta parmakla bulunur
    zs = ust_z(XB, YB)
    mercek = (cq.Workplane("XY").add(cq.Solid.makeSphere(12.0, cq.Vector(XB, YB, zs + 0.5 - 12.0), angleDegrees1=-90, angleDegrees2=90))
              .intersect(silindir(5.5, 0, 30, XB, YB)))
    return g.union(mercek)


def usb_yuva():
    return (cq.Workplane("YZ").center(0, USB_Z).rect(USB_G, USB_Y).extrude(7).edges("|X").fillet(3.2)
            .translate((-LC / 2 - 6.5, 0, 0)))


def tik(r, uz):
    """Uzun kenarlarda, dilin dış yüzünde X boyunca yarım silindir (iki yan)."""
    y = BC / 2 + DIL_T + TIK - 0.6
    g = None
    for s in (1, -1):
        c = cq.Workplane("YZ").center(s * y, ZS + 0.8).circle(r).extrude(uz).translate((-uz / 2, 0, 0))
        g = c if g is None else g.union(c)
    return g


def taban(d=None):
    d = d or dis()
    g = d.intersect(cq.Workplane("XY").box(200, 200, ZS, centered=(True, True, False)))
    g = g.union(ofset(DIL_T, ZS - 0.5, ZS + DIL_H))                        # geçme dili
    g = g.union(tik(0.6, 14.0).intersect(ofset(DIL_T + TIK, ZS, ZS + DIL_H)))
    g = g.cut(ofset(0, FL, ZS + DIL_H + 1))                                 # yığın boşluğu
    # ip halkası: +X ucunda yassı dil, dikey delik
    dil = (cq.Workplane("XY").center(A + 1.75, 0).slot2D(11.5, 9.0, 0).extrude(4.0).faces(">Z").edges().fillet(1.2))
    g = g.union(dil).cut(silindir(1.7, -1, 6, A + 3.0, 0))
    g = g.cut(cq.Workplane("XY").box(DIL_T + 3, USB_G, 5, centered=(False, True, False)).translate((-LC / 2 - 2, 0, ZS - 0.01)))  # yuva üstünde dil kalmasın
    return g.cut(usb_yuva())


def kapak(d=None):
    d = d or dis()
    g = d.intersect(cq.Workplane("XY").box(200, 200, 30, centered=(True, True, False)).translate((0, 0, ZS)))
    olu = ofset(DIL_T + DIL_B, ZS - 1, ZS + DIL_H + 0.15).cut(ofset(-CEKIRDEK, ZS - 1, ZS + DIL_H + 0.2))
    g = g.cut(olu)
    g = g.cut(tik(0.65, 15.0))                                              # tık yuvası
    g = g.cut(silindir(GOVDE_R, ZS - 1, GOVDE_UST, XB, YB))
    g = g.cut(silindir(CEP_R, ZS - 1, CEP_UST, XB, YB)).cut(silindir(DELIK_R, ZS, 30, XB, YB))
    return g.cut(usb_yuva())


def pim():
    tepe = ust_z(XB, YB) + 0.5 + 0.7                                        # merceğin 0.7 üstü
    g = silindir(YAKA_R, ZBT, ZBT + YAKA_H, XB, YB).union(silindir(MIL_R, ZBT + YAKA_H - 0.01, tepe, XB, YB))
    return g.faces(">Z").edges().fillet(1.2)


def hacim(w):
    return round(sum(s.Volume() for s in w.solids().vals()), 3)


if __name__ == "__main__":
    d = dis()
    t, k, p = taban(d), kapak(d), pim()
    for ad, w in (("taban", t), ("kapak", k), ("pim", p)):
        v = w.val(); bb = v.BoundingBox()
        print(ad, "geçerli", v.isValid(), "katı", len(w.solids().vals()),
              "boyut", round(bb.xlen, 1), round(bb.ylen, 1), round(bb.zlen, 1), "hacim", round(v.Volume() / 1000, 2), "cm3")
    yigin = (kutu_r(L, B, FL, FL + H, 2.0).union(kutu_r(6.0, 6.0, FL + H - 0.1, ZBT - 1.0, 0.3, XB, YB))
             .union(silindir(1.75, ZBT - 1.1, ZBT, XB, YB)))
    print("çakışma yığın∩taban", hacim(t.intersect(yigin)), "yığın∩kapak", hacim(k.intersect(yigin)),
          "taban∩kapak", hacim(t.intersect(k)), "pim∩kapak", hacim(p.intersect(k)), "pim∩yığın", hacim(p.intersect(yigin)))
    # duvar denetimleri
    print("taban yan duvar 1.2 taşma", hacim(ofset(1.2, FL, ZS).cut(d)))
    print("kapak oluk dış duvarı 0.8 taşma", hacim(ofset(DIL_T + DIL_B + 0.8, ZS, ZS + DIL_H + 0.15).cut(d)))
    print("cep üstü 1.0 taşma", hacim(silindir(CEP_R + 0.6, ZS, CEP_UST + 1.0, XB, YB).cut(d)))
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    for ad, w in (("taban", t), ("kapak", k), ("pim", p)):
        cq.exporters.export(w, f"{out}/case-{ {'taban': 'base', 'kapak': 'lid', 'pim': 'pin'}[ad] }.stl", tolerance=0.02, angularTolerance=0.08)
    cq.exporters.export(t.union(k).union(p), f"{out}/case-assembly.step")
    cq.exporters.export(yigin, f"{out}/_stack.stl", tolerance=0.05, angularTolerance=0.2)

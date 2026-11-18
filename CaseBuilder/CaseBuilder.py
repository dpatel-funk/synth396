# CaseBuilder.py - builds the synth case (base, lid, button caps) from the numbers below.
# Fusion X = KiCad X, Fusion Y = -KiCad Y, Z = 0 is the underside of the PCB.
# Change a number, run the script again: it deletes its own previous output and rebuilds.
import adsk.core, adsk.fusion, traceback

# ======================= PARAMETERS (mm) =======================
BOARD_X0, BOARD_X1 = 50.0, 150.0      # board outline in KiCad coordinates
BOARD_Y0, BOARD_Y1 = 50.0, 150.0
PCB_T = 1.6
CLEAR = 1.0            # gap between board edge and inside of wall
WALL = 2.4
FLOOR = 3.0
STANDOFF = 6.0         # post height under the board
LID_T = 2.4
CEIL = 14.0            # free height above the PCB top (lid underside)

POST_D, SCREW_D = 6.0, 3.4            # base posts and screw holes
PILOT_D, PILOT_DEPTH = 2.6, 12.0      # pilot holes in the lid pillars
HOLES = [(54.0, 52.0), (146.0, 52.0), (145.0, 145.0)]    # KiCad mounting holes

POTS = [(58.5 + 16.3 * i, 65.15) for i in range(6)]
POT_HOLE = 8.0
BTNS = [(59.25 + 14.0 * c, y) for y in (79.25, 89.75) for c in range(4)]
BTN_HOLE = 7.6
LED, LED_HOLE = (136.23, 114.0), 5.0
FSW, FSW_HOLE = (136.0, 133.0), 13.0   # footswitch: thread ~12 mm, hole oversize for print shrink
FSW_RAISE = 5.0                        # lid is raised this much over the footswitch (body ~16 mm deep, ceiling only 14 mm)
FSW_POCKET = (28.0, 17.0)              # inner size X, Y of the raised pocket (switch body ~25 x 14, long side along X)
FSW_POCKET_WALL = 2.4                  # wall of the raised pocket
FSW_SEPARATE_PAD = True                # False: pad is part of the lid (needs supports). True: lid gets a window and the pad is a separate part
                                       # that prints roof-down on the bed with no supports; glue it over the window.
PAD_PLACE = (100.0, 45.0)              # where the separate pad is laid out (Fusion X, Y), away from the case and caps

TEENSY_Y = 125.38                      # USB centre line (KiCad y)
USB_W, USB_H, USB_ZC = 12.0, 9.0, 9.0  # width, height, centre height above PCB top
JACK_YS = [86.0, 110.0]                # KiCad y of the two 1/4" jacks (right wall), 24 mm apart (washers are ~16 mm)
JACK_D, JACK_ZC = 10.0, 7.5            # 10 mm hole for the 3/8" bushing; turn the lug ears to point along the wall

BTN_TOP = 5.0          # height of the 6x6x5 mm tact switch above the PCB top (measure when they arrive)
CAP_STEM_D, CAP_FLANGE_D, CAP_CORE_D = 7.0, 9.4, 5.5   # flange must stay under 10.5 (button row pitch)
CAP_FLANGE_T = 1.6
CAP_GAP = 0.8          # space between cap flange and lid underside (covers button height tolerance)
CAP_STEM_OVER = 1.5    # stem sticks out above the lid top
CAP_ROW_Y = 20.0       # where the caps are placed (Fusion Y), away from the case

# ---- engraved labels (mm) ----
LABEL_DEPTH = 0.4
LABEL_FONT = 'Arial Black'               # bold = thicker grooves that print cleanly
POT_LABEL_H = 2.8                        # pot label height (5 letters must fit in 16.3 mm pitch)
LEGEND_H = 3.0                           # legend and FX ON height
POT_LABELS = ['LEVEL', 'TONE', 'MIX', 'TIME', 'PITCH', 'SYNTH']   # RV1..RV6
POT_LABEL_Y = 53.0                       # KiCad y of the pot labels (behind the knobs)
LEGEND = [('1 DLY  2 CHO', 98.0), ('3 DST  4 REV', 103.0), ('5-8 SYNTH', 108.0)]  # (text, KiCad y)
LEGEND_X = 78.0
FSW_LABEL = ('FX ON', 136.23, 107.8)     # text, KiCad x, KiCad y (just above the LED, clear of the raised footswitch pad)
CAP_DIGIT_H = 4.0

# ======================= derived =======================
PCB_TOP = PCB_T
Z_FLOOR_BOT = -(FLOOR + STANDOFF)
Z_FLOOR_TOP = -STANDOFF
Z_LID_U = PCB_TOP + CEIL
Z_LID_TOP = Z_LID_U + LID_T
IX0, IX1 = BOARD_X0 - CLEAR, BOARD_X1 + CLEAR
IY0, IY1 = BOARD_Y0 - CLEAR, BOARD_Y1 + CLEAR
OX0, OX1 = IX0 - WALL, IX1 + WALL
OY0, OY1 = IY0 - WALL, IY1 + WALL

NAMES_NEW = ('BASE CASE v2', 'CASE LID v2', 'BUTTON CAPS', 'FOOTSWITCH PAD')
NAMES_OLD = ('BASE CASE', 'CASE LID')


def cm(v):
    return v / 10.0


def new_comp(root, name):
    occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
    occ.component.name = name
    return occ.component


def z_plane(comp, z):
    if abs(z) < 1e-9:
        return comp.xYConstructionPlane
    pi = comp.constructionPlanes.createInput()
    pi.setByOffset(comp.xYConstructionPlane, adsk.core.ValueInput.createByReal(cm(z)))
    return comp.constructionPlanes.add(pi)


def x_plane(comp, x):
    pi = comp.constructionPlanes.createInput()
    pi.setByOffset(comp.yZConstructionPlane, adsk.core.ValueInput.createByReal(cm(x)))
    return comp.constructionPlanes.add(pi)


def pt(sk, x, y, z):
    p = sk.modelToSketchSpace(adsk.core.Point3D.create(cm(x), cm(y), cm(z)))
    p.z = 0
    return p


def rect(sk, x1, y1, x2, y2, z):
    sk.sketchCurves.sketchLines.addTwoPointRectangle(pt(sk, x1, y1, z), pt(sk, x2, y2, z))


def circle(sk, x, y, z, d):
    sk.sketchCurves.sketchCircles.addByCenterRadius(pt(sk, x, y, z), cm(d / 2.0))


def extrude(comp, sk, op, dist, symmetric=False):
    if sk.profiles.count == 0:
        raise RuntimeError('Sketch "%s" has no closed profile' % sk.name)
    coll = adsk.core.ObjectCollection.create()
    for i in range(sk.profiles.count):
        coll.add(sk.profiles.item(i))
    inp = comp.features.extrudeFeatures.createInput(coll, op)
    v = adsk.core.ValueInput.createByReal(cm(dist))
    if symmetric:
        inp.setSymmetricExtent(v, True)
    else:
        inp.setDistanceExtent(False, v)
    return comp.features.extrudeFeatures.add(inp)


NEW = adsk.fusion.FeatureOperations.NewBodyFeatureOperation
JOIN = adsk.fusion.FeatureOperations.JoinFeatureOperation
CUT = adsk.fusion.FeatureOperations.CutFeatureOperation


def build_base(root):
    c = new_comp(root, 'BASE CASE v2')
    # 1. solid block (floor + walls)
    sk = c.sketches.add(z_plane(c, Z_FLOOR_BOT))
    rect(sk, OX0, -OY1, OX1, -OY0, Z_FLOOR_BOT)
    f = extrude(c, sk, NEW, Z_LID_U - Z_FLOOR_BOT)
    f.bodies.item(0).name = 'Base'
    # 2. pocket for the electronics, open at the top
    sk = c.sketches.add(z_plane(c, Z_FLOOR_TOP))
    rect(sk, IX0, -IY1, IX1, -IY0, Z_FLOOR_TOP)
    extrude(c, sk, CUT, (Z_LID_U - Z_FLOOR_TOP) + 2.0)
    # 3. posts under the board
    sk = c.sketches.add(z_plane(c, Z_FLOOR_TOP))
    for (x, y) in HOLES:
        circle(sk, x, -y, Z_FLOOR_TOP, POST_D)
    extrude(c, sk, JOIN, STANDOFF)
    # 4. screw holes through floor + posts
    sk = c.sketches.add(z_plane(c, Z_FLOOR_BOT))
    for (x, y) in HOLES:
        circle(sk, x, -y, Z_FLOOR_BOT, SCREW_D)
    extrude(c, sk, CUT, (FLOOR + STANDOFF) + 0.5)
    # 5. USB cutout (left wall)
    xm = (OX0 + IX0) / 2.0
    zc = PCB_TOP + USB_ZC
    sk = c.sketches.add(x_plane(c, xm))
    sk.sketchCurves.sketchLines.addTwoPointRectangle(
        pt(sk, xm, -TEENSY_Y - USB_W / 2.0, zc - USB_H / 2.0),
        pt(sk, xm, -TEENSY_Y + USB_W / 2.0, zc + USB_H / 2.0))
    extrude(c, sk, CUT, WALL + 3.0, symmetric=True)
    # 6. jack holes (right wall)
    xm = (OX1 + IX1) / 2.0
    zc = PCB_TOP + JACK_ZC
    sk = c.sketches.add(x_plane(c, xm))
    for y in JACK_YS:
        sk.sketchCurves.sketchCircles.addByCenterRadius(pt(sk, xm, -y, zc), cm(JACK_D / 2.0))
    extrude(c, sk, CUT, WALL + 3.0, symmetric=True)
    return c


def build_lid(root):
    c = new_comp(root, 'CASE LID v2')
    # 1. plate
    sk = c.sketches.add(z_plane(c, Z_LID_U))
    rect(sk, OX0, -OY1, OX1, -OY0, Z_LID_U)
    f = extrude(c, sk, NEW, LID_T)
    f.bodies.item(0).name = 'Lid'
    # 2. pillars from the PCB top up into the plate (1 mm overlap so they merge)
    sk = c.sketches.add(z_plane(c, PCB_TOP))
    for (x, y) in HOLES:
        circle(sk, x, -y, PCB_TOP, POST_D)
    extrude(c, sk, JOIN, (Z_LID_U - PCB_TOP) + 1.0)
    # 3. pilot holes in the pillars
    sk = c.sketches.add(z_plane(c, PCB_TOP))
    for (x, y) in HOLES:
        circle(sk, x, -y, PCB_TOP, PILOT_D)
    extrude(c, sk, CUT, PILOT_DEPTH)
    # 4. holes through the plate
    sk = c.sketches.add(z_plane(c, Z_LID_U))
    for (x, y) in POTS:
        circle(sk, x, -y, Z_LID_U, POT_HOLE)
    for (x, y) in BTNS:
        circle(sk, x, -y, Z_LID_U, BTN_HOLE)
    circle(sk, LED[0], -LED[1], Z_LID_U, LED_HOLE)
    extrude(c, sk, CUT, LID_T + 1.0)
    # 5. raised pocket over the footswitch so its ~16 mm body clears the board
    pw, ph = FSW_POCKET
    ow = FSW_POCKET_WALL
    fx, fy = FSW
    if FSW_SEPARATE_PAD:
        # window through the plate; the separate pad is glued on top around it
        sk = c.sketches.add(z_plane(c, Z_LID_U))
        rect(sk, fx - pw / 2.0, -(fy + ph / 2.0), fx + pw / 2.0, -(fy - ph / 2.0), Z_LID_U)
        extrude(c, sk, CUT, LID_T + 1.0)
        return c
    sk = c.sketches.add(z_plane(c, Z_LID_U))
    rect(sk, fx - pw / 2.0 - ow, -(fy + ph / 2.0 + ow), fx + pw / 2.0 + ow, -(fy - ph / 2.0 - ow), Z_LID_U)
    extrude(c, sk, JOIN, FSW_RAISE + LID_T)              # outer block, top = lid top + FSW_RAISE
    sk = c.sketches.add(z_plane(c, Z_LID_U))
    rect(sk, fx - pw / 2.0, -(fy + ph / 2.0), fx + pw / 2.0, -(fy - ph / 2.0), Z_LID_U)
    extrude(c, sk, CUT, FSW_RAISE)                       # pocket, open to the underside
    z_top = Z_LID_U + FSW_RAISE
    sk = c.sketches.add(z_plane(c, z_top))
    circle(sk, fx, -fy, z_top, FSW_HOLE)
    extrude(c, sk, CUT, LID_T + 1.0)                     # switch hole through the pocket roof
    return c


def build_pad(root):
    """Separate footswitch pad, modelled roof-down (roof on the bed) so it prints with no supports.
    Glue it on the lid top around the window. Its height is FSW_RAISE, so the roof underside ends up
    FSW_RAISE above the lid underside, same as the integrated version."""
    c = new_comp(root, 'FOOTSWITCH PAD')
    pw, ph = FSW_POCKET
    ow = FSW_POCKET_WALL
    px, py = PAD_PLACE
    sk = c.sketches.add(c.xYConstructionPlane)
    rect(sk, px - pw / 2.0 - ow, py - ph / 2.0 - ow, px + pw / 2.0 + ow, py + ph / 2.0 + ow, 0.0)
    f = extrude(c, sk, NEW, FSW_RAISE)
    f.bodies.item(0).name = 'Footswitch pad'
    sk = c.sketches.add(z_plane(c, LID_T))               # cavity, open at the top (roof stays LID_T thick)
    rect(sk, px - pw / 2.0, py - ph / 2.0, px + pw / 2.0, py + ph / 2.0, LID_T)
    extrude(c, sk, CUT, FSW_RAISE)
    sk = c.sketches.add(c.xYConstructionPlane)           # switch hole through the roof
    circle(sk, px, py, 0.0, FSW_HOLE)
    extrude(c, sk, CUT, LID_T + 0.5)
    return c


def build_caps(root):
    c = new_comp(root, 'BUTTON CAPS')
    flange_top = (Z_LID_U - CAP_GAP) - (PCB_TOP + BTN_TOP)      # from plunger to flange top
    stem_len = (Z_LID_TOP + CAP_STEM_OVER) - (Z_LID_U - CAP_GAP)
    core_len = flange_top - CAP_FLANGE_T
    xs = [59.25 + 14.0 * i for i in range(8)]
    ov = 0.2   # overlap so the pieces merge
    # printed stem-down: stem at the bed, flange, then core pointing up
    sk = c.sketches.add(z_plane(c, 0.0))
    for x in xs:
        circle(sk, x, CAP_ROW_Y, 0.0, CAP_STEM_D)
    extrude(c, sk, NEW, stem_len)
    z1 = stem_len - ov
    sk = c.sketches.add(z_plane(c, z1))
    for x in xs:
        circle(sk, x, CAP_ROW_Y, z1, CAP_FLANGE_D)
    extrude(c, sk, JOIN, CAP_FLANGE_T + ov)
    z2 = stem_len + CAP_FLANGE_T - ov
    sk = c.sketches.add(z_plane(c, z2))
    for x in xs:
        circle(sk, x, CAP_ROW_Y, z2, CAP_CORE_D)
    extrude(c, sk, JOIN, core_len + ov)
    return c


FLIP_STATUS = []


def add_text(sk, text, cx, cy, z, h, w, flip=False):
    """Centered text in a box w x 2h around (cx, cy) in Fusion coordinates (mm)."""
    p1 = pt(sk, cx - w / 2.0, cy - h, z)
    p2 = pt(sk, cx + w / 2.0, cy + h, z)
    inp = sk.sketchTexts.createInput2(text, cm(h))
    inp.setAsMultiLine(p1, p2,
                       adsk.core.HorizontalAlignments.CenterHorizontalAlignment,
                       adsk.core.VerticalAlignments.MiddleVerticalAlignment, 0)
    try:
        inp.fontName = LABEL_FONT
    except:
        pass
    flipped = False
    if flip:
        try:
            inp.isHorizontalFlip = True
            flipped = True
        except:
            pass
    t = sk.sketchTexts.add(inp)
    if flip and not flipped:
        try:
            t.isHorizontalFlip = True
            flipped = True
        except:
            pass
    if flip:
        FLIP_STATUS.append(flipped)
    return t


def cut_texts(comp, sk, depth):
    coll = adsk.core.ObjectCollection.create()
    for i in range(sk.sketchTexts.count):
        coll.add(sk.sketchTexts.item(i))
    inp = comp.features.extrudeFeatures.createInput(coll, CUT)
    inp.setDistanceExtent(False, adsk.core.ValueInput.createByReal(cm(depth)))
    comp.features.extrudeFeatures.add(inp)


def engrave_lid(c):
    z = Z_LID_TOP - LABEL_DEPTH
    sk = c.sketches.add(z_plane(c, z))
    for i, name in enumerate(POT_LABELS):
        add_text(sk, name, POTS[i][0], -POT_LABEL_Y, z, POT_LABEL_H, 15.0)
    for (txt, y) in LEGEND:
        add_text(sk, txt, LEGEND_X, -y, z, LEGEND_H, 50.0)
    add_text(sk, FSW_LABEL[0], FSW_LABEL[1], -FSW_LABEL[2], z, LEGEND_H, 20.0)
    cut_texts(c, sk, LABEL_DEPTH + 1.0)


def engrave_caps(c):
    # caps are modelled stem-down for printing, so the digit is on the bed-side face: mirror it
    sk = c.sketches.add(z_plane(c, 0.0))
    for i in range(8):
        add_text(sk, str(i + 1), 59.25 + 14.0 * i, CAP_ROW_Y, 0.0, CAP_DIGIT_H, 6.0, flip=True)
    cut_texts(c, sk, LABEL_DEPTH)


def run(context):
    app = adsk.core.Application.get()
    ui = app.userInterface
    try:
        design = adsk.fusion.Design.cast(app.activeProduct)
        root = design.rootComponent
        old = [root.occurrences.item(i) for i in range(root.occurrences.count)]
        for o in old:
            try:
                n = o.component.name
                if n.startswith(NAMES_NEW):          # also catches renamed copies like 'CASE LID v2 (1)'
                    o.deleteMe()
                elif n in NAMES_OLD:
                    o.isLightBulbOn = False
            except:
                pass
        build_base(root)
        lid = build_lid(root)
        caps = build_caps(root)
        if FSW_SEPARATE_PAD:
            build_pad(root)
        notes = ''
        try:
            engrave_lid(lid)
        except:
            notes += '\nLid labels FAILED:\n' + traceback.format_exc()
        try:
            engrave_caps(caps)
            if FLIP_STATUS and not all(FLIP_STATUS):
                notes += '\nCap digits may be mirrored (flip not supported): check them.'
        except:
            notes += '\nCap digit labels FAILED:\n' + traceback.format_exc()
        ui.messageBox(
            'Case built.\n\n'
            'Lid underside: %.1f mm above PCB top (Z=%.1f), lid top Z=%.1f\n'
            'Walls: Z=%.1f to %.1f\n'
            'Old BASE CASE / CASE LID are hidden, not deleted.\n'
            'Caps are laid out at Y=%.0f, away from the case.'
            % (CEIL, Z_LID_U, Z_LID_TOP, Z_FLOOR_BOT, Z_LID_U, CAP_ROW_Y) + notes)
    except:
        ui.messageBox('CaseBuilder failed:\n%s' % traceback.format_exc())

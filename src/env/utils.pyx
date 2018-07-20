ctypedef double fpn_t

cdef extern from "math.h":
    fpn_t hypot(fpn_t, fpn_t)
    fpn_t sqrt(fpn_t)
    fpn_t sin(fpn_t)
    fpn_t asin(fpn_t)
    fpn_t cos(fpn_t)
    fpn_t acos(fpn_t)
    fpn_t atan2(fpn_t, fpn_t)
    fpn_t log(fpn_t)
    fpn_t fabs(fpn_t)

cdef fpn_t calc_v(fpn_t *vxy):
    return hypot(vxy[0], vxy[1])

cdef fpn_t calc_r(fpn_t *a, fpn_t *b):
    return hypot(b[0] - a[0], b[1] - a[1])

cdef fpn_t calc_th_1(fpn_t *a):
    return atan2(a[0], a[1])

cdef fpn_t calc_th_2(fpn_t *a, fpn_t *b):
    return atan2(b[0] - a[0], b[1] - a[1])

# configuration of playground
cdef fpn_t STONE_RADIUS = 0.145
cdef fpn_t HOUSE_RADIUS = 1.83

cdef fpn_t W_SPIN = 0.066696

cdef fpn_t PLAYAREA_WIDTH = 4.75
cdef fpn_t PLAYAREA_LENGTH = 8.23

cdef fpn_t X_TEE = PLAYAREA_WIDTH / 2
cdef fpn_t Y_TEE = 3.05 + HOUSE_RADIUS

cdef fpn_t X_PLAYAREA_MIN = X_TEE - PLAYAREA_WIDTH / 2
cdef fpn_t X_PLAYAREA_MAX = X_TEE + PLAYAREA_WIDTH / 2
cdef fpn_t Y_PLAYAREA_MIN = Y_TEE + HOUSE_RADIUS - PLAYAREA_LENGTH
cdef fpn_t Y_PLAYAREA_MAX = Y_TEE + HOUSE_RADIUS

cdef fpn_t X_THROW = X_TEE
cdef fpn_t Y_THROW = 41.28 # Y_PLAYAREA_MIN + 30.0

cdef fpn_t XY_TEE[2]
XY_TEE[0] = X_TEE; XY_TEE[1] = Y_TEE
cdef fpn_t XY_THROW[2]
XY_THROW[0] = X_THROW; XY_THROW[1] = Y_THROW


# # configuration of physics
# simulation constant
cdef fpn_t FRICTION_RINK = 12.009216 # ( = g * mu )
cdef fpn_t FRICTION_STONES = FRICTION_RINK # strange setting in official rule

cdef fpn_t DX_V_R = 0.001386965639075
cdef fpn_t DY_V_R = 0.041588442394742

#DR_V_R = np.hypot(DX_V_R, DY_V_R)
cdef fpn_t DR_V_R = 0.041611563471

cdef fpn_t PHI = atan2(DY_V_R, DX_V_R)

cdef fpn_t ALPHA = 0.03333564

#B = (math.cos(ALPHA) ** 2) / (1 - (math.cos(ALPHA) ** 2))
cdef fpn_t B = 29.986811440344486

#A = DX_V_R * (dc.calc_r(dc.XY_TEE, dc.XY_THROW) / DR_V_R) / (math.cos(PHI) * math.exp(SPIRAL_B * PHI))
cdef fpn_t A = 3.45628911574e-19

cdef fpn_t R_COLLISION = 0.000004
cdef fpn_t DT_COLLISION = 0.000001

cdef fpn_t MAT_ALPHA[2][2][2]
MAT_ALPHA[0][0][0] = cos(-ALPHA); MAT_ALPHA[0][0][1] = -sin(-ALPHA); MAT_ALPHA[0][1][0] = sin(-ALPHA); MAT_ALPHA[0][1][1] = cos(-ALPHA)
MAT_ALPHA[1][0][0] = cos(ALPHA); MAT_ALPHA[1][0][1] = -sin(ALPHA); MAT_ALPHA[1][1][0] = sin(ALPHA); MAT_ALPHA[1][1][1] = cos(ALPHA)

"""
    vec -> pos
"""
cdef calc_dxy_by_vxys(fpn_t *vxy, int spin):
    cdef fpn_t dxy[2]
    dxy[0] = DR_V_R * calc_v(vxy) * (MAT_ALPHA[spin][0][0] * vxy[0] + MAT_ALPHA[spin][0][1] * vxy[1])
    dxy[1] = DR_V_R * calc_v(vxy) * (MAT_ALPHA[spin][1][0] * vxy[0] + MAT_ALPHA[spin][1][1] * vxy[1])
    return dxy

cdef calc_xy_by_xy_vxyw(fpn_t *oxy, fpn_t *vxy, fpn_t w):
    cdef fpn_t dxy[2]
    dxy = calc_dxy_by_vxys(vxy, int(w < 0))
    return oxy[0] + dxy[0], oxy[1] + dxy[1]

cdef calc_xy_by_vxyw_from_throw(fpn_t *vxy, fpn_t w):
    return calc_xy_by_xy_vxyw(XY_THROW, vxy, w)

cdef vec_to_pos(fpn_t vx, fpn_t vy, int spin):
    cdef fpn_t xy[2]
    if spin == 1:
        xy = calc_xy_by_vxyw_from_throw([vx, vy], W_SPIN)
        return [xy[0], xy[1], 1]
    else:
        xy = calc_xy_by_vxyw_from_throw([vx, vy], -W_SPIN)
        return [xy[0], xy[1], 0]

"""
    pos -> vec
"""
cdef fpn_t calc_v_by_r(fpn_t r):
    return sqrt(r / DR_V_R)

cdef calc_vtheta_by_theta(fpn_t theta, fpn_t w):
    if w < 0:
        return theta + ALPHA
    else:
        return theta - ALPHA

cdef calc_vxy_by_xyw(fpn_t *oxy, fpn_t *xy, fpn_t w):
    cdef fpn_t v, theta, vtheta, vx
    v = calc_v_by_r(calc_r(oxy, xy))
    theta = calc_th_2(oxy, xy)
    vtheta = calc_vtheta_by_theta(theta, w)
    vx = v * sin(vtheta)
    return vx, v * cos(vtheta)

cdef calc_vxy_by_xyw_from_throw(fpn_t *xy, fpn_t w):
    return calc_vxy_by_xyw(XY_THROW, xy, w)

cdef pos_to_vec(fpn_t x, fpn_t y, int spin):
    cdef fpn_t vxy[2]
    if spin == 1:
        vxy = calc_vxy_by_xyw_from_throw([x, y], W_SPIN)
        return [vxy[0], vxy[1], 1]
    else:
        vxy = calc_vxy_by_xyw_from_throw([x, y], -W_SPIN)
        return [vxy[0], vxy[1], 0]


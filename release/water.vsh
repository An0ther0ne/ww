vs.1.0
;------------------------------------------------------------------------------
; Constants specified by the app
;    c0-c3   = world matrix  
;    c4-c7   = view matrix
;    c8-c11  = projection matrix
;    c12     = light direction
;
;    v0      = Position
;    v3	     = Normal
;    v5      = Color
;    v7      = Texture Coord
;------------------------------------------------------------------------------
;Простое освещение
;------------------------------------------------------------------------------
m4x4 r0,   v0,   c4	; Transform to view space
m4x4 oPos, r0,   c8	; Transform to projection space
dp3  r0.x, v3,   c12    ; perform lighting N dot L calculation in world space
mul  oD0,  r0.x, v5     ; calculate final pixel color from light intensity and Store output color
mov  oT0,  v7		; move texture color to output texture register

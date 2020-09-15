/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

/****** HARD-CODED SCRIPT BYTECODES ******/
/*

/// SIN WAVES ///

// (LET ((distance-to-src (LET ((n (INIT-FEEDBACK REF((FUN () (INF)) 0)))) (FEEDBACK n (MUX (RED (LET ((t (INIT-FEEDBACK REF((FUN () 0.0) 1)))) (FEEDBACK t (IF (BUTTON 0.0) (- 1.0 t) t)))) 0.0 (B+ 1.0 (FOLD-HOOD REF((FUN (a0 a1) (MIN a0 a1)) 2) (INF) n))))))) (IF (GREEN (> distance-to-src 20.0)) (MOVE 0.5) (MOVE (B+ 0.5 (B* 0.2 (SIN (B+ (LET ((t (INIT-FEEDBACK REF((FUN () 0.0) 1)))) (FEEDBACK t (FOLD-HOOD-PLUS REF((FUN (a0 a1) (MAX a0 a1)) 3) REF((FUN (e) (B+ e (NBR-LAG))) 4) t))) (B* 0.5 distance-to-src))))))))
uint8_t script[] = { DEF_VM_OP, 2, 2, 0, 6, 3, 0, 16, 7, DEF_FUN_2_OP, INF_OP, RET_OP, DEF_FUN_2_OP, LIT_0_OP, RET_OP, DEF_FUN_4_OP, REF_1_OP, REF_0_OP, MIN_OP, RET_OP, DEF_FUN_4_OP, REF_1_OP, REF_0_OP, MAX_OP, RET_OP, DEF_FUN_4_OP, REF_0_OP, NBR_LAG_OP, ADD_OP, RET_OP, DEF_FUN_OP, 90, GLO_REF_0_OP, INIT_FEEDBACK_OP, 0, LET_1_OP, REF_0_OP, GLO_REF_1_OP, INIT_FEEDBACK_OP, 1, LET_1_OP, REF_0_OP, LIT_0_OP, BUTTON_OP, IF_OP, 3, REF_0_OP, JMP_OP, 3, LIT_1_OP, REF_0_OP, SUB_OP, FEEDBACK_OP, 1, POP_LET_1_OP, RED_OP, LIT_0_OP, LIT_1_OP, GLO_REF_2_OP, INF_OP, REF_0_OP, FOLD_HOOD_OP, 0, ADD_OP, MUX_OP, FEEDBACK_OP, 0, POP_LET_1_OP, LET_1_OP, REF_0_OP, LIT8_OP, 20, GT_OP, GREEN_OP, IF_OP, 38, LIT_FLO_OP, 0, 0, 0, 63, LIT_FLO_OP, 0, 0, 128, 62, GLO_REF_1_OP, INIT_FEEDBACK_OP, 2, LET_1_OP, REF_0_OP, GLO_REF_3_OP, GLO_REF_OP, 4, REF_0_OP, FOLD_HOOD_PLUS_OP, 1, FEEDBACK_OP, 2, POP_LET_1_OP, LIT_FLO_OP, 0, 0, 0, 63, REF_0_OP, MUL_OP, ADD_OP, SIN_OP, MUL_OP, ADD_OP, MOVE_OP, JMP_OP, 6, LIT_FLO_OP, 0, 0, 0, 63, MOVE_OP, POP_LET_1_OP, RET_OP, EXIT_OP };
uint16_t script_len = 123;


// (let ((dist (hop-gradient (red (toggle (button 0))))) (t (sync-time))) (move (if (green (> dist 8)) 0.5 (* 0.25 (+ 1 (sin (+ t (if (< dist 4) 3.145 0) (* 0.5 dist)))))))
// (LET ((dist (LET ((n (INIT-FEEDBACK REF((FUN () (INF)) 0)))) (FEEDBACK n (MUX (RED (LET ((t (INIT-FEEDBACK REF((FUN () 0.0) 1)))) (FEEDBACK t (IF (BUTTON 0.0) (- 1.0 t) t)))) 0.0 (B+ 1.0 (FOLD-HOOD REF((FUN (a0 a1) (MIN a0 a1)) 2) (INF) n))))))) (MOVE (IF (GREEN (> dist 8.0)) 0.5 (B* 0.2 (B+ 1.0 (SIN (B+ (LET ((t (INIT-FEEDBACK REF((FUN () 0.0) 1)))) (FEEDBACK t (FOLD-HOOD-PLUS REF((FUN (a0 a1) (MAX a0 a1)) 3) REF((FUN (e) (B+ e (NBR-LAG))) 4) t))) (B+ (IF (< dist 4.0) 3.1 0.0) (B* 0.5 dist)))))))))
uint8_t script[] = { DEF_VM_OP, 2, 2, 0, 6, 3, 0, 16, 7, DEF_FUN_2_OP, INF_OP, RET_OP, DEF_FUN_2_OP, LIT_0_OP, RET_OP, DEF_FUN_4_OP, REF_1_OP, REF_0_OP, MIN_OP, RET_OP, DEF_FUN_4_OP, REF_1_OP, REF_0_OP, MAX_OP, RET_OP, DEF_FUN_4_OP, REF_0_OP, NBR_LAG_OP, ADD_OP, RET_OP, DEF_FUN_OP, 99, GLO_REF_0_OP, INIT_FEEDBACK_OP, 0, LET_1_OP, REF_0_OP, GLO_REF_1_OP, INIT_FEEDBACK_OP, 1, LET_1_OP, REF_0_OP, LIT_0_OP, BUTTON_OP, IF_OP, 3, REF_0_OP, JMP_OP, 3, LIT_1_OP, REF_0_OP, SUB_OP, FEEDBACK_OP, 1, POP_LET_1_OP, RED_OP, LIT_0_OP, LIT_1_OP, GLO_REF_2_OP, INF_OP, REF_0_OP, FOLD_HOOD_OP, 0, ADD_OP, MUX_OP, FEEDBACK_OP, 0, POP_LET_1_OP, LET_1_OP, REF_0_OP, LIT8_OP, 8, GT_OP, GREEN_OP, IF_OP, 47, LIT_FLO_OP, 0, 0, 128, 62, LIT_1_OP, GLO_REF_1_OP, INIT_FEEDBACK_OP, 2, LET_1_OP, REF_0_OP, GLO_REF_3_OP, GLO_REF_OP, 4, REF_0_OP, FOLD_HOOD_PLUS_OP, 1, FEEDBACK_OP, 2, POP_LET_1_OP, REF_0_OP, LIT_4_OP, LT_OP, IF_OP, 3, LIT_0_OP, JMP_OP, 5, LIT_FLO_OP, 174, 71, 73, 64, LIT_FLO_OP, 0, 0, 0, 63, REF_0_OP, MUL_OP, ADD_OP, ADD_OP, SIN_OP, ADD_OP, MUL_OP, JMP_OP, 5, LIT_FLO_OP, 0, 0, 0, 63, MOVE_OP, POP_LET_1_OP, RET_OP, EXIT_OP };
uint16_t script_len = 132;

// (LET ((source (RED (LET ((t (INIT-FEEDBACK REF((FUN () 0.0) 3)))) (FEEDBACK t (IF (BUTTON 0.0) (- 1.0 t) t))))) (servo-pos (BEARING))) (LET ((distance (LET ((n (INIT-FEEDBACK REF((FUN () (INF)) 4)))) (FEEDBACK n (MUX source 0.0 (B+ 1.0 (FOLD-HOOD REF((FUN (a0 a1) (MIN a0 a1)) 5) (INF) n))))))) (LET ((servo-target (INIT-FEEDBACK REF((FUN () 0.5) 6)))) (MOVE (FEEDBACK servo-target (MUX source servo-pos (ELT (VFOLD-HOOD REF((FUN (r x) (IF (< (ELT x 0.0) (ELT r 0.0)) x r)) 7) (TUP (INF) servo-pos) (TUP distance servo-target)) 1.0)))))))
uint8_t script[] = { DEF_VM_OP, 4, 2, 0, 9, 3, 0, 20, 9, DEF_NUM_VEC_2_OP, DEF_NUM_VEC_2_OP, DEF_NUM_VEC_2_OP, DEF_FUN_2_OP, LIT_0_OP, RET_OP, DEF_FUN_2_OP, INF_OP, RET_OP, DEF_FUN_4_OP, REF_1_OP, REF_0_OP, MIN_OP, RET_OP, DEF_FUN_6_OP, LIT_FLO_OP, 0, 0, 0, 63, RET_OP, DEF_FUN_OP, 14, REF_0_OP, LIT_0_OP, ELT_OP, REF_1_OP, LIT_0_OP, ELT_OP, LT_OP, IF_OP, 3, REF_1_OP, JMP_OP, 1, REF_0_OP, RET_OP, DEF_FUN_OP, 75, GLO_REF_3_OP, INIT_FEEDBACK_OP, 0, LET_1_OP, REF_0_OP, LIT_0_OP, BUTTON_OP, IF_OP, 3, REF_0_OP, JMP_OP, 3, LIT_1_OP, REF_0_OP, SUB_OP, FEEDBACK_OP, 0, POP_LET_1_OP, RED_OP, BEARING_OP, LET_2_OP, GLO_REF_OP, 4, INIT_FEEDBACK_OP, 1, LET_1_OP, REF_0_OP, REF_2_OP, LIT_0_OP, LIT_1_OP, GLO_REF_OP, 5, INF_OP, REF_0_OP, FOLD_HOOD_OP, 0, ADD_OP, MUX_OP, FEEDBACK_OP, 1, POP_LET_1_OP, LET_1_OP, GLO_REF_OP, 6, INIT_FEEDBACK_OP, 2, LET_1_OP, REF_0_OP, REF_3_OP, REF_2_OP, GLO_REF_OP, 7, INF_OP, REF_2_OP, TUP_OP, 0, 2, REF_1_OP, REF_0_OP, TUP_OP, 1, 2, VFOLD_HOOD_OP, 2, 1, LIT_1_OP, ELT_OP, MUX_OP, FEEDBACK_OP, 2, MOVE_OP, POP_LET_1_OP, POP_LET_1_OP, POP_LET_2_OP, RET_OP, EXIT_OP };
uint16_t script_len = 124;



/// BEARING GRADCAST ///

// (move (hop-gradcast (red (toggle (button 0))) (bearing)))
// (MOVE (LET ((src (RED (LET ((t (INIT-FEEDBACK REF((FUN () 0.0) 3)))) (FEEDBACK t (IF (BUTTON 0.0) (- 1.0 t) t))))) (f (BEARING))) (LET ((d (LET ((n (INIT-FEEDBACK REF((FUN () (INF)) 4)))) (FEEDBACK n (MUX src 0.0 (B+ 1.0 (FOLD-HOOD REF((FUN (a0 a1) (MIN a0 a1)) 5) (INF) n))))))) (LET ((v (INIT-FEEDBACK REF((FUN () f) 6)))) (FEEDBACK v (MUX src f (ELT (VFOLD-HOOD REF((FUN (r x) (IF (< (ELT x 0.0) (ELT r 0.0)) x r)) 7) (TUP (INF) f) (TUP d v)) 1.0)))))))
uint8_t script[] = { DEF_VM_OP, 4, 2, 0, 9, 3, 0, 20, 9, DEF_NUM_VEC_2_OP, DEF_NUM_VEC_2_OP, DEF_NUM_VEC_2_OP, DEF_FUN_2_OP, LIT_0_OP, RET_OP, DEF_FUN_2_OP, INF_OP, RET_OP, DEF_FUN_4_OP, REF_1_OP, REF_0_OP, MIN_OP, RET_OP, DEF_FUN_2_OP, REF_1_OP, RET_OP, DEF_FUN_OP, 14, REF_0_OP, LIT_0_OP, ELT_OP, REF_1_OP, LIT_0_OP, ELT_OP, LT_OP, IF_OP, 3, REF_1_OP, JMP_OP, 1, REF_0_OP, RET_OP, DEF_FUN_OP, 75, GLO_REF_3_OP, INIT_FEEDBACK_OP, 0, LET_1_OP, REF_0_OP, LIT_0_OP, BUTTON_OP, IF_OP, 3, REF_0_OP, JMP_OP, 3, LIT_1_OP, REF_0_OP, SUB_OP, FEEDBACK_OP, 0, POP_LET_1_OP, RED_OP, BEARING_OP, LET_2_OP, GLO_REF_OP, 4, INIT_FEEDBACK_OP, 1, LET_1_OP, REF_0_OP, REF_2_OP, LIT_0_OP, LIT_1_OP, GLO_REF_OP, 5, INF_OP, REF_0_OP, FOLD_HOOD_OP, 0, ADD_OP, MUX_OP, FEEDBACK_OP, 1, POP_LET_1_OP, LET_1_OP, GLO_REF_OP, 6, INIT_FEEDBACK_OP, 2, LET_1_OP, REF_0_OP, REF_3_OP, REF_2_OP, GLO_REF_OP, 7, INF_OP, REF_2_OP, TUP_OP, 0, 2, REF_1_OP, REF_0_OP, TUP_OP, 1, 2, VFOLD_HOOD_OP, 2, 1, LIT_1_OP, ELT_OP, MUX_OP, FEEDBACK_OP, 2, POP_LET_1_OP, POP_LET_1_OP, POP_LET_2_OP, MOVE_OP, RET_OP, EXIT_OP };
uint16_t script_len = 120;

*/

/*
uint8_t script[] = { DEF_VM_OP, 0, 0, 0, 1, 0, 0, 1, 1, DEF_FUN_3_OP, LIT_1_OP, FLEX_OP, RET_OP, EXIT_OP };
uint16_t script_len = 14;
*/

//(flex (if (toggle (button 0)) (red 1) -0.5))
uint8_t script[] = { DEF_VM_OP, 0, 0, 0, 2, 1, 0, 9, 3, DEF_FUN_2_OP, LIT_0_OP, RET_OP, DEF_FUN_OP, 31, GLO_REF_0_OP, INIT_FEEDBACK_OP, 0, LET_1_OP, REF_0_OP, LIT_0_OP, BUTTON_OP, IF_OP, 3, REF_0_OP, JMP_OP, 3, LIT_1_OP, REF_0_OP, SUB_OP, FEEDBACK_OP, 0, POP_LET_1_OP, IF_OP, 7, LIT_FLO_OP, 0, 0, 0, 191, JMP_OP, 2, LIT_1_OP, RED_OP, FLEX_OP, RET_OP, EXIT_OP };
uint16_t script_len = 46;


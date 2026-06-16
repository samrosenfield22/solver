

#include "play_windows.h"
#include "solver.h"
#include "../utils/utils.h"

int analysis_hdl = -1, eval_hdl = -1, notation_win_hdl = -1, p1_window_hdl=-1, p2_window_hdl=-1;

void init_play_windows(void)
{
	if(analysis_hdl == -1)
		analysis_hdl = window_wh(117, 3, 49, 40);
	if(eval_hdl == -1)
		eval_hdl = window_wh(50, 3, 50, 7 + DISPLAY_VAR_CT);

	if(notation_win_hdl == -1)
	{
		notation_win_hdl = window_wh(18, 3, 20, 20);
		window_set_colors(TERM_CYAN, TERM_BLACK_BG);
	}

	if(p1_window_hdl == -1)
	{
		p1_window_hdl = window_wh(56, 38, 10, 8);
		window_set_colors(TERM_YELLOW, TERM_BLACK_BG);
		window_set_border(NO_BORDER_STYLE);
	}
	if(p2_window_hdl == -1)
	{
		p2_window_hdl = window_wh(56+25, 38, 10, 8);
		window_set_colors(TERM_YELLOW, TERM_BLACK_BG);
		window_set_border(NO_BORDER_STYLE);
	}

	//default window
	window_focus(analysis_hdl);
}

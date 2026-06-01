

#include "play_windows.h"
#include "../../utils.h"

int analysis_hdl = -1, eval_hdl = -1, notation_win_hdl = -1;

void init_play_windows(void)
{
	if(analysis_hdl == -1)
		analysis_hdl = window_wh(117, 3, 48, 69);
	if(eval_hdl == -1)
		eval_hdl = window_wh(52, 3, 46, 7);

	if(notation_win_hdl == -1)
	{
		notation_win_hdl = window_wh(2, 2, 20, 20);
		window_set_colors(TERM_CYAN, TERM_BLACK_BG);
	}
}

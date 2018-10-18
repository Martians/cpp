
#pragma once

/**
 * check if testing now
 **/
inline bool Testing(int set = -1) {
	static bool s_testing = 0;
	if (set == -1) {
		return s_testing;
	}
	s_testing = (set != 0);
	return s_testing;
}

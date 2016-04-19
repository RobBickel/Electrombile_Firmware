/*
 * fs.h
 *
 *  Created on: 2016年2月19日
 *      Author: jk
 */

#ifndef USER_INC_FS_H_
#define USER_INC_FS_H_


#define SETTINGFILE_NAME  L"C:\\setting.conf"

void fs_initial(void);
SINT64 fs_getDiskFreeSize(void);


#endif /* USER_INC_FS_H_ */

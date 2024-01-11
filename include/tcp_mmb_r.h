#ifndef TCP_MMB_r_H
#define TCP_MMB_r_H
#include <unordered_map>
#include <vector>
#include "daappconfigs.h"

#include "pai_r_data.h"


#ifdef __cplusplus
extern "C"
{
#endif

int tcp_mmb_init(void);
int tcp_mmb_send(CPai_r_data &pai_r_data, bool send_require, eUserDataType ud_type);

#ifdef __cplusplus
}
#endif

#endif // TCP_MMB_r_H
#include "optimizer.h"


void optimize()
{
    optimize_DAG();     //构建DAG图优化并且重新生成中间代码
    optimize_del();     //删除中间代码中的无用代码并且重新生成中间代码
}
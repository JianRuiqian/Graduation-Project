#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>

#ifdef RT_USING_DFS_MNTTABLE
const struct dfs_mount_tbl mount_table[] =
{
    {"flash0", "/", "elm", 0, 0},
    {0}
};
#endif  /* RT_USING_DFS_MNTTABLE */

#endif  /* RT_USING_DFS */

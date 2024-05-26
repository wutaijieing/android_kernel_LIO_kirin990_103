#ifndef __KERNEL__
    #define __KERNEL__
#endif

#ifndef MODULE
    #define MODULE
#endif
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "Kunit.h"
#include "Stub.h"

#define OSP_S32 int
#define OSP_VOID void

static OSP_S32 g_iExpected = 0;

extern OSP_S32 LAYOUT_ModuleInit(OSP_VOID);
extern OSP_S32 LAYOUT_MemPoolInit(OSP_VOID);
extern OSP_S32 LAYOUT_AllLunLayOutInit(OSP_VOID);
extern OSP_S32 LAYOUT_LunMemIdentIdInit(OSP_VOID);
extern OSP_S32 LAYOUT_NumForMapInit(OSP_VOID);
 
OSP_S32 LAYOUT_MemPoolInit_Stub1(OSP_VOID)
{
    return 0;	
}

OSP_S32 LAYOUT_MemPoolInit_Stub2(OSP_VOID)
{
    return -1;	
}

OSP_VOID LAYOUT_AllLunLayOutInit_Stub(OSP_VOID)
{
    return;	
}

OSP_VOID LAYOUT_LunMemIdentIdInit_Stub(OSP_VOID)
{
    return;	
}

OSP_VOID LAYOUT_NumForMapInit_Stub(OSP_VOID)
{
    return;
}

void LAYOUT_ModuleInit_test1(void)
{
    printk("here is LAYOUT_ModuleInit test!\n");

    SET_FUNC_STUB(LAYOUT_MemPoolInit, LAYOUT_MemPoolInit_Stub1);
    SET_FUNC_STUB(LAYOUT_AllLunLayOutInit, LAYOUT_AllLunLayOutInit_Stub);
    SET_FUNC_STUB(LAYOUT_LunMemIdentIdInit, LAYOUT_LunMemIdentIdInit_Stub);
    SET_FUNC_STUB(LAYOUT_NumForMapInit, LAYOUT_NumForMapInit_Stub);
    g_iExpected = LAYOUT_ModuleInit();
    
    CLEAR_FUNC_STUB(LAYOUT_MemPoolInit);
    CLEAR_FUNC_STUB(LAYOUT_AllLunLayOutInit);    
    CLEAR_FUNC_STUB(LAYOUT_LunMemIdentIdInit);
    CLEAR_FUNC_STUB(LAYOUT_NumForMapInit);  

    CHECK_EQUAL(0, g_iExpected);
}

void LAYOUT_ModuleInit_test2(void)
{
    printk("here is LAYOUT_ModuleInit2 test!\n");

    SET_FUNC_STUB(LAYOUT_MemPoolInit, LAYOUT_MemPoolInit_Stub2);
    SET_FUNC_STUB(LAYOUT_AllLunLayOutInit, LAYOUT_AllLunLayOutInit_Stub);
    SET_FUNC_STUB(LAYOUT_LunMemIdentIdInit, LAYOUT_LunMemIdentIdInit_Stub);
    SET_FUNC_STUB(LAYOUT_NumForMapInit, LAYOUT_NumForMapInit_Stub);
    g_iExpected = LAYOUT_ModuleInit();
    
    CLEAR_FUNC_STUB(LAYOUT_MemPoolInit);
    CLEAR_FUNC_STUB(LAYOUT_AllLunLayOutInit);    
    CLEAR_FUNC_STUB(LAYOUT_LunMemIdentIdInit);
    CLEAR_FUNC_STUB(LAYOUT_NumForMapInit);  

    CHECK_EQUAL(-1, g_iExpected);
}

KU_TestInfo LAYOUT_ModuleInit_testcase1[]=
{
    	{"LAYOUT_ModuleInit_test1",LAYOUT_ModuleInit_test1},
    	KU_TEST_INFO_NULL
};

KU_TestInfo LAYOUT_ModuleInit_testcase2[]=
{
    	{"LAYOUT_ModuleInit_test2",LAYOUT_ModuleInit_test2},
    	KU_TEST_INFO_NULL
};

int suite_init(void)
{  
    return 0;
}

int suite_clean(void)
{
    return 0;
}

KU_SuiteInfo suites[]=
{
    	{"suite_first",suite_init,suite_clean,LAYOUT_ModuleInit_testcase1,KU_TRUE},
       {"suite_sencond",suite_init,suite_clean,LAYOUT_ModuleInit_testcase2,KU_FALSE},		
	KU_SUITE_INFO_NULL		
};

static int kunit_init(void)
{
    printk("hello,kunit test!\n");
    RUN_ALL_TESTS(suites,"layout");
    return 0;
}

static void kunit_exit(void)
{
    printk("bye,kunit test!\n");
}

module_init(kunit_init);
module_exit(kunit_exit);

MODULE_LICENSE("GPL");





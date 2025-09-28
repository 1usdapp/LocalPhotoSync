#ifndef WSMACROS_H 
#define WSMACROS_H




#define WS_FUNC(varType, varName, funName)\
protected: varType varName;\
public: varType Get##funName(void) const { return varName; }\
public: void Set##funName(varType var){ varName = var; }

#define WS_FUNC_INIT(varType, varName , varInitValue, funName)\
protected: varType varName=varInitValue;\
public: varType Get##funName(void) const { return varName; }\
public: void Set##funName(varType var){ varName = var; }

#define WS_FUNC_READ(varType, varName, funName)\
protected: varType varName;\
public: varType Get##funName(void) const { return varName; }

#define LINFO \
LOG(INFO) << __FUNCTION__ << "\t"

#define LWARNING \
LOG(WARNING) << __FUNCTION__ << "\t" 

#define LERROR \
LOG(ERROR) << __FUNCTION__ << "\t" 

#define LFATAL \
LOG(FATAL) << __FUNCTION__ << "\t" 


#define RECV_BUFF_LEN	65535





#endif
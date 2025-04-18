 #pragma once

#include <ostream>
#include "CoreMinimal.h"
#include "Logging/StructuredLog.h"

/* Custom log category */

DECLARE_LOG_CATEGORY_EXTERN(LogAnchor, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogPhysicsEditor, Log, All);

#pragma once

//Current Class Name + Function Name where this is called!
#define TRACE_STR_CUR_CLASS_FUNC (FString(__FUNCTION__))

//Current Class where this is called!
#define TRACE_STR_CUR_CLASS (FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT(":"))) )

//Current Function Name where this is called!
#define TRACE_STR_CUR_FUNC (FString(__FUNCTION__).Right(FString(__FUNCTION__).Len() - FString(__FUNCTION__).Find(TEXT("::")) - 2 ))

//Current Line Number in the code where this is called!
#define TRACE_STR_CUR_LINE  (FString::FromInt(__LINE__))

//Current Class and Line Number where this is called!
#define TRACE_STR_CUR_CLASS_LINE (TRACE_STR_CUR_CLASS + "(" + TRACE_STR_CUR_LINE + ")")

//Current Class Name + Function Name and Line Number where this is called!
#define TRACE_STR_CUR_CLASS_FUNC_LINE (TRACE_STR_CUR_CLASS_FUNC + "(" + TRACE_STR_CUR_LINE + ")")

//Current Function Signature where this is called!
#define TRACE_STR_CUR_FUNCSIG (FString(__FUNCSIG__))

#define LOG_CAT LogAnchor
//Screen Message
#define TRACE_SCREENMSG(OutputMessage) (GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, *(TRACE_STR_CUR_CLASS_FUNC_LINE + ": " + OutputMessage)) )
#define TRACE_SCREENMSG_PRINTF(FormatString , ...) (GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, *(TRACE_STR_CUR_CLASS_FUNC_LINE + ": " + (FString::Printf(TEXT(FormatString), ##__VA_ARGS__ )))) )

#define LGV(FormatString , ...) UE_LOG(LOG_CAT, Verbose, TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) ) 
#define LG(FormatString , ...) UE_LOG(LOG_CAT, Log, TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) ) 
#define LGW(FormatString , ...) UE_LOG(LOG_CAT, Warning, TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) ) 
#define LGE(FormatString , ...) UE_LOG(LOG_CAT, Error, TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) ) 
#define LOG(OutputMessage) UE_LOG(LogAnchor,Display,TEXT("%s"), *FString(OutputMessage))
#define LOG_F(FormatString , ...) UE_LOG(LogAnchor,Display,TEXT("%s"), *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) )

#define LOG_WARN(OutputMessage) UE_LOG(LogAnchor,Warning,TEXT("%s"), *FString(OutputMessage))
#define LOG_WARN_F(FormatString , ...) UE_LOG(LogAnchor,Warning,TEXT("%s"), *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) )

#define LOG_ERR(OutputMessage) UE_LOG(LogAnchor,Error,TEXT("%s"), *FString(OutputMessage))
#define LOG_ERR_F(FormatString , ...) UE_LOG(LogAnchor,Error,TEXT("%s"), *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) )

//UE LOG!
#define TRACE_LOG(LogCategory, OutputMessage) UE_LOG(LogCategory,Log,TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString(OutputMessage))
#define TRACE_LOG_PRINTF(LogCat, FormatString , ...) UE_LOG(LogCat,Log,TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) )

#define TRACE_WARNING(LogCategory, OutputMessage) UE_LOG(LogCategory,Warning,TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString(OutputMessage))
#define TRACE_WARNING_PRINTF(LogCategory, FormatString , ...) UE_LOG(LogCategory,Warning,TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) )

#define TRACE_ERROR(LogCategory, OutputMessage) UE_LOG(LogCategory,Error,TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString(OutputMessage))
#define TRACE_ERROR_PRINTF(LogCategory, FormatString , ...) UE_LOG(LogCategory,Error,TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) )

#define TRACE_FATAL(LogCategory, OutputMessage) UE_LOG(LogCategory,Fatal,TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString(OutputMessage))
#define TRACE_FATAL_PRINTF(LogCategory, FormatString , ...) UE_LOG(LogCategory,Fatal,TEXT("%s: %s"), *TRACE_STR_CUR_CLASS_FUNC_LINE, *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) )


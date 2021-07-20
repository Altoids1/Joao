#pragma once

enum class LocalType {
	Value,
	Number,
	Object,
	Boolean,
	String,
	Local
};

enum class ErrorCode : int {
	NoError, // This should always be the 0th one
	Unknown, // and this should always be the 1st one.
	FailedTypecheck,
	FailedOperation,
	BadBreak,
	BadCall,
	BadArgType,
	NotEnoughArgs,
	BadMemberAccess,
	BadAccess
};
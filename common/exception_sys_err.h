//
// Copyright (c) 2021 Paul Ranson, paul@epicyclism.com
//
//
#pragma once

// extend std::exception to contain error codes

#include <exception>
#include <string>
#include <cstring>

template<typename ErrCodeType>
class exceptionError : public std::exception
{
private:
	ErrCodeType err_;
	char sMsg_[1024];

public:
	exceptionError(ErrCodeType err, char const* sContext) 
	{
		err_ = err;
#if defined(_WIN32)
		strcpy_s(sMsg_, sizeof(sMsg_), sContext);
#else
		strcpy(sMsg_, sContext);
#endif
	}

	exceptionError(ErrCodeType err, char const* sContext, char const* sDetail)
	{
		err_ = err;
#if defined(_WIN32)
		strcpy_s(sMsg_, sizeof(sMsg_), sContext);
		strcat_s(sMsg_, sizeof(sMsg_), sDetail);
#else
		strcpy(sMsg_, sContext);
		strcat(sMsg_, sDetail);
#endif
	}

	ErrCodeType Get() const
	{
		return err_;
	}
	virtual const char* what() const noexcept
	{
		return sMsg_;
	}
};

#if defined(_WIN32)
using exceptionSystemError = exceptionError<unsigned long>;
#else
using exceptionSystemError = exceptionError<int>;
#endif

template<typename ErrCodeType>
class exceptionErrorStr : public std::exception
{
private:
	ErrCodeType err_;
	std::string msg_;

public:
	exceptionErrorStr(ErrCodeType err, std::string_view sContext) : err_{err}, msg_{sContext}
	{
	}
	exceptionErrorStr(ErrCodeType err, std::string&& sContext) : err_{err}, msg_{sContext}
	{
	}

	ErrCodeType get() const
	{
		return err_;
	}
	virtual const char* what() const noexcept
	{
		return msg_.c_str();
	}
};

#if defined(_WIN32)
using exceptionSystemErrorStr = exceptionErrorStr< long>;
#else
using exceptionSystemErrorStr = exceptionErrorStr<int>;
#endif

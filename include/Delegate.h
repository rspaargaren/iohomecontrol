/*
   Copyright (c) 2024. CRIDP https://github.com/cridp

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

           http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * Delegate.h
 *
 ****/

/** @defgroup   delegate Delegate
 *  @brief      Delegates are event handlers
 *  @{
 */
// #pragma once
#ifndef DELEGATE_H
#define DELEGATE_H

#include <functional>
using namespace std::placeholders;

template <typename> class Delegate; /* undefined */

/** @brief  Delegate class
*/
template <typename ReturnType, typename... ParamTypes>
class Delegate<ReturnType(ParamTypes...)> : public std::function<ReturnType(ParamTypes...)> {
	using StdFunc = std::function<ReturnType(ParamTypes...)>;

public:
	using StdFunc::function;

	Delegate() = default;

	/** @brief  Delegate a class method
	 *  @param m Method declaration to delegate
	 *  @param  c Pointer to the class type
	 */
	template <class ClassType>
	Delegate(ReturnType (ClassType::*m)(ParamTypes...), ClassType* c)
		: StdFunc([m, c](ParamTypes... params) -> ReturnType { return (c->*m)(params...); })
	{
	}
};

/** @} */
#endif // DELEGATE_H

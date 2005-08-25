/*
 * kdomxpath.h - Copyright 2005 Frerich Raabe <raabe@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef KDOMXPATH_H
#define KDOMXPATH_H

namespace KDOM
{
	namespace XPath
	{
		enum XPathExceptionCode {
			INVALID_EXPRESSION_ERR = 1,
			TYPE_ERR = 2
		};

		enum XPathResultType {
			ANY_TYPE = 0,
			NUMBER_TYPE = 1,
			STRING_TYPE = 2,
			BOOLEAN_TYPE = 3,
			UNORDERED_NODE_ITERATOR_TYPE = 4,
			ORDERED_NODE_ITERATOR_TYPE = 5,
			UNORDERED_NODE_SNAPSHOT_TYPE = 6,
			ORDERED_NODE_SNAPSHOT_TYPE = 7,
			ANY_UNORDERED_NODE_TYPE = 8,
			FIRST_ORDERED_NODE_TYPE = 9
		};

		enum XPathNodeType {
			XPATH_NAMESPACE_NODE = 13
		};
	}
}

#endif // KDOMXPATH_H


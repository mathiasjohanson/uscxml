/**
 *  @file
 *  @author     2012-2014 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */

// bison -v -d promela-expr.ypp && flex promela-expr.l
// bison promela-expr.ypp && flex promela-expr.l

#ifndef PROMELA_H_9AB78YB1
#define PROMELA_H_9AB78YB1

#include <stdlib.h>
#include <stdarg.h>

#include "uscxml/Message.h"

namespace uscxml {

class PromelaParser;

struct PromelaParserNode {
	PromelaParserNode() : type(0) {}
	virtual ~PromelaParserNode();

	void merge(PromelaParserNode* node);
	void push(PromelaParserNode* node);
	void dump(int indent = 0);

	static std::string typeToDesc(int type);

	int type;
	std::string value;
	std::list<PromelaParserNode*> operands;

};

class PromelaParser {
public:
	enum Type {
	    PROMELA_EXPR,
	    PROMELA_DECL,
	    PROMELA_STMNT
	};

	static std::string typeToDesc(int type);

	PromelaParser(const std::string& expr);
	PromelaParser(const std::string& expr, Type expectedType);
	virtual ~PromelaParser();

	virtual PromelaParserNode* node(int type, int nrArgs, ...);
	virtual PromelaParserNode* value(int type, const char* value);
	void dump();


	PromelaParserNode* ast;
	Type type;

	Event pendingException;

protected:

	void init(const std::string& expr);
	void destroy();

	void* buffer;
	void* scanner;
	char* input;
	size_t input_length;
};

}

void promela_error (uscxml::PromelaParser* ctx, void* yyscanner, const char* err);

#endif /* end of include guard: PROMELA_H_9AB78YB1 */

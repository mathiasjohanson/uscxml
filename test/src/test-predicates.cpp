#define protected public
#include "uscxml/Interpreter.h"
#undef protected

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Expected path to test-predicates.scxml" << std::endl;
		exit(EXIT_FAILURE);
	}

	using namespace uscxml;
	using namespace Arabica::DOM;
	using namespace Arabica::XPath;

	Interpreter interpreter = Interpreter::fromURI(argv[1]);
	assert(interpreter);

	Node<std::string> atomicState = interpreter.getState("atomic");
	assert(InterpreterImpl::isAtomic(atomicState));
	assert(!InterpreterImpl::isParallel(atomicState));
	assert(!InterpreterImpl::isCompound(atomicState));

	Node<std::string> compoundState = interpreter.getState("compound");
	assert(!InterpreterImpl::isAtomic(compoundState));
	assert(!InterpreterImpl::isParallel(compoundState));
	assert(InterpreterImpl::isCompound(compoundState));

	Node<std::string> parallelState = interpreter.getState("parallel");
	assert(!InterpreterImpl::isAtomic(parallelState));
	assert(InterpreterImpl::isParallel(parallelState));
	assert(!InterpreterImpl::isCompound(parallelState)); // parallel states are not compound!

	NodeSet<std::string> initialState = interpreter.getImpl()->getInitialStates();
	assert(initialState[0] == atomicState);

	NodeSet<std::string> childs = interpreter.getImpl()->getChildStates(compoundState);
	Node<std::string> compoundChild1 = interpreter.getState("compoundChild1");
	Node<std::string> compoundChild2 = interpreter.getState("compoundChild2");
	assert(childs.size() > 0);
	assert(Interpreter::isMember(compoundChild1, childs));
	assert(Interpreter::isMember(compoundChild2, childs));
	assert(!Interpreter::isMember(compoundState, childs));

	assert(InterpreterImpl::isDescendant(compoundChild1, compoundState));

	{
		std::string idrefs("id1");
		std::list<std::string> tokenizedIdrefs = InterpreterImpl::tokenizeIdRefs(idrefs);
		assert(tokenizedIdrefs.size() == 1);
		assert(tokenizedIdrefs.front().compare("id1") == 0);
	}

	{
		std::string idrefs(" id1");
		std::list<std::string> tokenizedIdrefs = InterpreterImpl::tokenizeIdRefs(idrefs);
		assert(tokenizedIdrefs.size() == 1);
		assert(tokenizedIdrefs.front().compare("id1") == 0);
	}

	{
		std::string idrefs(" id1 ");
		std::list<std::string> tokenizedIdrefs = InterpreterImpl::tokenizeIdRefs(idrefs);
		assert(tokenizedIdrefs.size() == 1);
		assert(tokenizedIdrefs.front().compare("id1") == 0);
	}

	{
		std::string idrefs(" \tid1\n ");
		std::list<std::string> tokenizedIdrefs = InterpreterImpl::tokenizeIdRefs(idrefs);
		assert(tokenizedIdrefs.size() == 1);
		assert(tokenizedIdrefs.front().compare("id1") == 0);
	}

	{
		std::string idrefs("id1 id2 id3");
		std::list<std::string> tokenizedIdrefs = InterpreterImpl::tokenizeIdRefs(idrefs);
		assert(tokenizedIdrefs.size() == 3);
		assert(tokenizedIdrefs.front().compare("id1") == 0);
		tokenizedIdrefs.pop_front();
		assert(tokenizedIdrefs.front().compare("id2") == 0);
		tokenizedIdrefs.pop_front();
		assert(tokenizedIdrefs.front().compare("id3") == 0);
	}

	{
		std::string idrefs("\t  id1 \nid2\n\n id3\t");
		std::list<std::string> tokenizedIdrefs = InterpreterImpl::tokenizeIdRefs(idrefs);
		assert(tokenizedIdrefs.size() == 3);
		assert(tokenizedIdrefs.front().compare("id1") == 0);
		tokenizedIdrefs.pop_front();
		assert(tokenizedIdrefs.front().compare("id2") == 0);
		tokenizedIdrefs.pop_front();
		assert(tokenizedIdrefs.front().compare("id3") == 0);
	}

	{
		std::string idrefs("id1 \nid2  \tid3");
		std::list<std::string> tokenizedIdrefs = InterpreterImpl::tokenizeIdRefs(idrefs);
		assert(tokenizedIdrefs.size() == 3);
		assert(tokenizedIdrefs.front().compare("id1") == 0);
		tokenizedIdrefs.pop_front();
		assert(tokenizedIdrefs.front().compare("id2") == 0);
		tokenizedIdrefs.pop_front();
		assert(tokenizedIdrefs.front().compare("id3") == 0);
	}

	std::string transEvents;
	transEvents = "error";
	assert(InterpreterImpl::nameMatch(transEvents, "error"));
	assert(!InterpreterImpl::nameMatch(transEvents, "foo"));

	transEvents = "error foo";
	assert(InterpreterImpl::nameMatch(transEvents, "error"));
	assert(InterpreterImpl::nameMatch(transEvents, "error.send"));
	assert(InterpreterImpl::nameMatch(transEvents, "error.send.failed"));
	assert(InterpreterImpl::nameMatch(transEvents, "foo"));
	assert(InterpreterImpl::nameMatch(transEvents, "foo.bar"));
	assert(!InterpreterImpl::nameMatch(transEvents, "errors.my.custom"));
	assert(!InterpreterImpl::nameMatch(transEvents, "errorhandler.mistake"));
	// is the event name case sensitive?
	//	assert(!InterpreterImpl::nameMatch(transEvents, "errOr.send"));
	assert(!InterpreterImpl::nameMatch(transEvents, "foobar"));
}
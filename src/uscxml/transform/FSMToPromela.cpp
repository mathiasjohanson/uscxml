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

#include "uscxml/transform/ChartToFSM.h"
#include "uscxml/transform/FSMToPromela.h"
#include "uscxml/plugins/datamodel/promela/PromelaParser.h"
#include <DOM/io/Stream.hpp>
#include <iostream>
#include "uscxml/UUID.h"
#include <math.h>
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>

namespace uscxml {

using namespace Arabica::DOM;
using namespace Arabica::XPath;

void FSMToPromela::writeProgram(std::ostream& stream,
                                const Interpreter& interpreter) {
	FSMToPromela promelaWriter;
	interpreter.getImpl()->copyTo(&promelaWriter);
	promelaWriter.writeProgram(stream);
}

FSMToPromela::FSMToPromela() : _eventTrie(".") {
}

void PromelaEventSource::writeStartEventSources(std::ostream& stream, int indent) {
	std::string padding;
	for (int i = 0; i < indent; i++) {
		padding += "  ";
	}

	std::list<PromelaInline>::iterator sourceIter = eventSources.inlines.begin();
	int i = 0;
	while(sourceIter != eventSources.inlines.end()) {
		if (sourceIter->type != PromelaInline::PROMELA_EVENT_SOURCE_CUSTOM && sourceIter->type != PromelaInline::PROMELA_EVENT_SOURCE) {
			sourceIter++;
			continue;
		}
		std::string sourceName = name + "_"+ toStr(i);
		stream << padding << "run " << sourceName << "EventSource();" << std::endl;

		i++;
		sourceIter++;
	}

}

void PromelaEventSource::writeStopEventSources(std::ostream& stream, int indent) {
	std::string padding;
	for (int i = 0; i < indent; i++) {
		padding += "  ";
	}

	std::list<PromelaInline>::iterator sourceIter = eventSources.inlines.begin();
	int i = 0;
	while(sourceIter != eventSources.inlines.end()) {
		if (sourceIter->type != PromelaInline::PROMELA_EVENT_SOURCE_CUSTOM && sourceIter->type != PromelaInline::PROMELA_EVENT_SOURCE) {
			sourceIter++;
			continue;
		}
		std::string sourceName = name + "_"+ toStr(i);
		stream << padding << sourceName << "EventSourceDone = 1;" << std::endl;

		i++;
		sourceIter++;
	}

}

void PromelaEventSource::writeDeclarations(std::ostream& stream, int indent) {
	std::string padding;
	for (int i = 0; i < indent; i++) {
		padding += "  ";
	}

	std::list<PromelaInline>::iterator sourceIter = eventSources.inlines.begin();
	int i = 0;
	while(sourceIter != eventSources.inlines.end()) {
		if (sourceIter->type != PromelaInline::PROMELA_EVENT_SOURCE_CUSTOM && sourceIter->type != PromelaInline::PROMELA_EVENT_SOURCE) {
			sourceIter++;
			continue;
		}
		std::string sourceName = name + "_"+ toStr(i);
		stream << "bool " << sourceName << "EventSourceDone = 0;" << std::endl;

		i++;
		sourceIter++;
	}
}

void PromelaEventSource::writeEventSource(std::ostream& stream) {

	std::list<PromelaInline>::iterator sourceIter = eventSources.inlines.begin();
	int i = 0;
	while(sourceIter != eventSources.inlines.end()) {
		if (sourceIter->type != PromelaInline::PROMELA_EVENT_SOURCE_CUSTOM && sourceIter->type != PromelaInline::PROMELA_EVENT_SOURCE) {
			sourceIter++;
			continue;
		}

		std::string sourceName = name + "_"+ toStr(i);

		stream << "proctype " << sourceName << "EventSource() {" << std::endl;
		stream << "  " << sourceName << "EventSourceDone = 0;" << std::endl;
		stream << "  " << sourceName << "NewEvent:" << std::endl;
		stream << "  " << "if" << std::endl;
		stream << "  " << ":: " << sourceName << "EventSourceDone -> skip;" << std::endl;
		stream << "  " << ":: else { " << std::endl;

		if (sourceIter->type == PromelaInline::PROMELA_EVENT_SOURCE_CUSTOM) {
			std::string content = sourceIter->content;

			boost::replace_all(content, "#REDO#", sourceName + "NewEvent");
			boost::replace_all(content, "#DONE#", sourceName + "Done");

			std::list<TrieNode*> eventNames = trie->getChildsWithWords(trie->getNodeWithPrefix(""));
			std::list<TrieNode*>::iterator eventNameIter = eventNames.begin();
			while(eventNameIter != eventNames.end()) {
				boost::replace_all(content, "#" + (*eventNameIter)->value + "#", "e" + toStr((*eventNameIter)->identifier));
				eventNameIter++;
			}

			stream << FSMToPromela::beautifyIndentation(content, 2) << std::endl;

		} else {
			stream << "  " << "  if" << std::endl;
//			stream << "  " << "  :: 1 -> " << "goto " << sourceName << "NewEvent;" << std::endl;

			std::list<std::list<std::string> >::const_iterator seqIter = sourceIter->sequences.begin();
			while(seqIter != sourceIter->sequences.end()) {
				stream << "    " << ":: ";
				std::list<std::string>::const_iterator evIter = seqIter->begin();
				while(evIter != seqIter->end()) {
					TrieNode* node = trie->getNodeWithPrefix(*evIter);
					stream << "eQ!" << node->identifier << "; ";
					evIter++;
				}
				stream << "goto " << sourceName << "NewEvent;" << std::endl;
				seqIter++;
			}

			stream << "  " << "  fi;" << std::endl;
		}

		stream << "  " << "}" << std::endl;
		stream << "  " << "fi;" << std::endl;
		stream << sourceName << "Done:" << " skip;" << std::endl;
		stream << "}" << std::endl;

		i++;
		sourceIter++;
	}
}

PromelaEventSource::PromelaEventSource() {
	type = PROMELA_EVENT_SOURCE_INVALID;
	trie = NULL;
}

PromelaEventSource::PromelaEventSource(const PromelaInlines& sources, const Arabica::DOM::Node<std::string>& parent) {
	type = PROMELA_EVENT_SOURCE_INVALID;
	trie = NULL;

	eventSources = sources;
	container = parent;
}

void FSMToPromela::writeEvents(std::ostream& stream) {
	std::list<TrieNode*> eventNames = _eventTrie.getWordsWithPrefix("");
	std::list<TrieNode*>::iterator eventIter = eventNames.begin();
	stream << "// event name identifiers" << std::endl;
	while(eventIter != eventNames.end()) {
		stream << "#define " << "e" << (*eventIter)->identifier << " " << (*eventIter)->identifier;
		stream << " // from \"" << (*eventIter)->value << "\"" << std::endl;
		eventIter++;
	}
}

void FSMToPromela::writeStates(std::ostream& stream) {
	stream << "// state name identifiers" << std::endl;
	for (int i = 0; i < _globalStates.size(); i++) {
		stream << "#define " << "s" << i << " " << i;
		stream << " // from \"" << ATTR(_globalStates[i], "id") << "\"" << std::endl;
	}

}

Arabica::XPath::NodeSet<std::string> FSMToPromela::getTransientContent(const Arabica::DOM::Node<std::string>& state) {
	Arabica::XPath::NodeSet<std::string> content;
	Arabica::DOM::Node<std::string> currState = state;
	for (;;) {
		if (!HAS_ATTR(currState, "transient") || !DOMUtils::attributeIsTrue(ATTR(currState, "transient")))
			break;
		content.push_back(filterChildElements(_nsInfo.xmlNSPrefix + "invoke", currState));
		content.push_back(filterChildElements(_nsInfo.xmlNSPrefix + "onentry", currState));
		content.push_back(filterChildElements(_nsInfo.xmlNSPrefix + "onexit", currState));
		NodeSet<std::string> transitions = filterChildElements(_nsInfo.xmlNSPrefix + "transition", currState);
		currState = _states[ATTR(transitions[0], "target")];
	}

	return content;
}

Node<std::string> FSMToPromela::getUltimateTarget(const Arabica::DOM::Node<std::string>& transition) {
	Arabica::DOM::Node<std::string> currState = _states[ATTR(transition, "target")];

	for (;;) {
		if (!HAS_ATTR(currState, "transient") || !DOMUtils::attributeIsTrue(ATTR(currState, "transient")))
			return currState;
		NodeSet<std::string> transitions = filterChildElements(_nsInfo.xmlNSPrefix + "transition", currState);
		currState = _states[ATTR(transitions[0], "target")];
	}
}

void FSMToPromela::writeInlineComment(std::ostream& stream, const Arabica::DOM::Node<std::string>& node) {
	if (node.getNodeType() != Node_base::COMMENT_NODE)
		return;

	std::string comment = node.getNodeValue();
	boost::trim(comment);
	if (!boost::starts_with(comment, "promela-inline:"))
		return;

	std::stringstream ssLine(comment);
	std::string line;
	std::getline(ssLine, line); // consume first line
	while(std::getline(ssLine, line)) {
		if (line.length() == 0)
			continue;
		stream << line;
	}
}

void FSMToPromela::writeExecutableContent(std::ostream& stream, const Arabica::DOM::Node<std::string>& node, int indent) {

	std::string padding;
	for (int i = 0; i < indent; i++) {
		padding += "  ";
	}

	if (node.getNodeType() == Node_base::COMMENT_NODE) {
		std::string comment = node.getNodeValue();
		boost::trim(comment);
		std::stringstream inlinePromela;
		if (!boost::starts_with(comment, "promela-inline:"))
			return;
		std::stringstream ssLine(comment);
		std::string line;
		std::getline(ssLine, line); // consume first line
		while(std::getline(ssLine, line)) {
			if (line.length() == 0)
				continue;
			inlinePromela << line << std::endl;
		}
		stream << padding << "skip;" << std::endl;
		stream << beautifyIndentation(inlinePromela.str(), indent) << std::endl;
	}

	if (node.getNodeType() != Node_base::ELEMENT_NODE)
		return;

	if (false) {
	} else if(TAGNAME(node) == "state") {
		if (HAS_ATTR(node, "transient") && DOMUtils::attributeIsTrue(ATTR(node, "transient"))) {
			Arabica::XPath::NodeSet<std::string> execContent = getTransientContent(node);
			for (int i = 0; i < execContent.size(); i++) {
				writeExecutableContent(stream, execContent[i], indent);
			}
		} else {
			Arabica::DOM::Node<std::string> child = node.getFirstChild();
			while(child) {
				writeExecutableContent(stream, child, indent);
				child = child.getNextSibling();
			}
		}
	} else if(TAGNAME(node) == "transition") {
		stream << "t" << _transitions[node] << ":" << std::endl;

		// check for special promela labels
		PromelaInlines promInls = getInlinePromela(getTransientContent(_states[ATTR(node, "target")]), true);

		if (promInls.acceptLabels > 0)
			stream << padding << "acceptLabelT" << _transitions[node] << ":" << std::endl;
		if (promInls.endLabels > 0)
			stream << padding << "endLabelT" << _transitions[node] << ":" << std::endl;
		if (promInls.progressLabels > 0)
			stream << padding << "progressLabelT" << _transitions[node] << ":" << std::endl;

		stream << padding << "atomic {" << std::endl;
		writeExecutableContent(stream, _states[ATTR(node, "target")], indent+1);
		stream << padding << "  skip;" << std::endl;

		Node<std::string> newState = getUltimateTarget(node);
		for (int i = 0; i < _globalStates.size(); i++) {
			if (newState != _globalStates[i])
				continue;
			stream << padding << "  s = s" << i << ";" << std::endl;
		}

		stream << padding << "}" << std::endl;
		if (isFinal(newState)) {
			stream << padding << "goto terminate;" << std::endl;
		} else {
			stream << padding << "goto nextStep;" << std::endl;
		}

	} else if(TAGNAME(node) == "onentry" || TAGNAME(node) == "onexit") {
		Arabica::DOM::Node<std::string> child = node.getFirstChild();
		while(child) {
			writeExecutableContent(stream, child, indent);
			child = child.getNextSibling();
		}

	} else if(TAGNAME(node) == "script") {
		NodeSet<std::string> scriptText = filterChildType(Node_base::TEXT_NODE, node, true);
		for (int i = 0; i < scriptText.size(); i++) {
			stream << beautifyIndentation(scriptText[i].getNodeValue(), indent) << std::endl;
		}

	} else if(TAGNAME(node) == "log") {
		// ignore

	} else if(TAGNAME(node) == "foreach") {
		if (HAS_ATTR(node, "index"))
			stream << padding << ATTR(node, "index") << " = 0;" << std::endl;
		stream << padding << "for (" << ATTR(node, "item") << " in " << ATTR(node, "array") << ") {" << std::endl;
		Arabica::DOM::Node<std::string> child = node.getFirstChild();
		while(child) {
			writeExecutableContent(stream, child, indent + 1);
			child = child.getNextSibling();
		}
		if (HAS_ATTR(node, "index"))
			stream << padding << "  " << ATTR(node, "index") << "++;" << std::endl;
		stream << padding << "}" << std::endl;

	} else if(TAGNAME(node) == "if") {
		NodeSet<std::string> condChain;
		condChain.push_back(node);
		condChain.push_back(filterChildElements(_nsInfo.xmlNSPrefix + "elseif", node));
		condChain.push_back(filterChildElements(_nsInfo.xmlNSPrefix + "else", node));

		writeIfBlock(stream, condChain, indent);

	} else if(TAGNAME(node) == "raise") {
		TrieNode* trieNode = _eventTrie.getNodeWithPrefix(ATTR(node, "event"));
		stream << padding << "iQ!e" << trieNode->identifier << ";" << std::endl;
	} else if(TAGNAME(node) == "send") {
		if (!HAS_ATTR(node, "target")) {
			// this is for our external queue
			TrieNode* trieNode = _eventTrie.getNodeWithPrefix(ATTR(node, "event"));
			stream << padding << "tmpQ!e" << trieNode->identifier << ";" << std::endl;
		}
	} else if(TAGNAME(node) == "invoke") {
		_invokers[ATTR(node, "invokeid")].writeStartEventSources(stream, indent);
	} else if(TAGNAME(node) == "uninvoke") {
		stream << padding << ATTR(node, "invokeid") << "EventSourceDone" << "= 1;" << std::endl;
	} else {

		std::cerr << "'" << TAGNAME(node) << "'" << std::endl << node << std::endl;
		assert(false);
	}

}

PromelaInlines FSMToPromela::getInlinePromela(const std::string& content) {
	PromelaInlines prom;

	std::stringstream ssLine(content);
	std::string line;

	bool isInPromelaCode = false;
	bool isInPromelaEventSource = false;
	PromelaInline promInl;

	while(std::getline(ssLine, line)) {
		std::string trimLine = boost::trim_copy(line);
		if (trimLine.length() == 0)
			continue;
		if (boost::starts_with(trimLine, "#promela")) {
			if (isInPromelaCode || isInPromelaEventSource) {
				prom.inlines.push_back(promInl);
				isInPromelaCode = false;
				isInPromelaEventSource = false;
			}
			promInl = PromelaInline();
		}

		if (false) {
		} else if (boost::starts_with(trimLine, "#promela-progress")) {
			prom.progressLabels++;
			promInl.type = PromelaInline::PROMELA_PROGRESS_LABEL;
			promInl.content = line;
			prom.inlines.push_back(promInl);
		} else if (boost::starts_with(trimLine, "#promela-accept")) {
			prom.acceptLabels++;
			promInl.type = PromelaInline::PROMELA_ACCEPT_LABEL;
			promInl.content = line;
			prom.inlines.push_back(promInl);
		} else if (boost::starts_with(trimLine, "#promela-end")) {
			prom.endLabels++;
			promInl.type = PromelaInline::PROMELA_END_LABEL;
			promInl.content = line;
			prom.inlines.push_back(promInl);
		} else if (boost::starts_with(trimLine, "#promela-inline")) {
			prom.codes++;
			isInPromelaCode = true;
			promInl.type = PromelaInline::PROMELA_CODE;
		} else if (boost::starts_with(trimLine, "#promela-event-source-custom")) {
			prom.customEventSources++;
			isInPromelaCode = true;
			promInl.type = PromelaInline::PROMELA_EVENT_SOURCE_CUSTOM;
		} else if (boost::starts_with(trimLine, "#promela-event-source")) {
			prom.eventSources++;
			isInPromelaEventSource = true;
			promInl.type = PromelaInline::PROMELA_EVENT_SOURCE;
		} else if (isInPromelaCode) {
			promInl.content += line;
			promInl.content += "\n";
		} else if (isInPromelaEventSource) {
			std::list<std::string> seq;
			std::stringstream ssToken(trimLine);
			std::string token;
			while(std::getline(ssToken, token, ' ')) {
				if (token.length() == 0)
					continue;
				seq.push_back(token);
			}
			promInl.sequences.push_back(seq);
		}
	}
	// inline code ends with comment
	if (isInPromelaCode || isInPromelaEventSource) {
		prom.inlines.push_back(promInl);
	}

	return prom;
}

PromelaInlines FSMToPromela::getInlinePromela(const Arabica::DOM::Node<std::string>& node) {
	if (node.getNodeType() != Node_base::COMMENT_NODE)
		return getInlinePromela(std::string());
	return getInlinePromela(node.getNodeValue());
}

PromelaInlines FSMToPromela::getInlinePromela(const Arabica::XPath::NodeSet<std::string>& elements, bool recurse) {
	PromelaInlines allPromInls;

	Arabica::XPath::NodeSet<std::string> comments = filterChildType(Node_base::COMMENT_NODE, elements, recurse);
	for (int i = 0; i < comments.size(); i++) {
		allPromInls.merge(getInlinePromela(comments[i]));
	}
	return allPromInls;
}

void FSMToPromela::writeIfBlock(std::ostream& stream, const Arabica::XPath::NodeSet<std::string>& condChain, int indent) {
	if (condChain.size() == 0)
		return;

	std::string padding;
	for (int i = 0; i < indent; i++) {
		padding += "  ";
	}

	bool noNext = condChain.size() == 1;
	bool nextIsElse = false;
	if (condChain.size() > 1) {
		if (TAGNAME(condChain[1]) == "else") {
			nextIsElse = true;
		}
	}

	Node<std::string> ifNode = condChain[0];

	stream << padding << "if" << std::endl;
	// we need to nest the elseifs to resolve promela if semantics
	stream << padding << ":: (" << ATTR(ifNode, "cond") << ") -> {" << std::endl;

	Arabica::DOM::Node<std::string> child;
	if (TAGNAME(ifNode) == "if") {
		child = ifNode.getFirstChild();
	} else {
		child = ifNode.getNextSibling();
	}
	while(child) {
		if (child.getNodeType() == Node_base::ELEMENT_NODE) {
			if (TAGNAME(child) == "elseif" || TAGNAME(child) == "else")
				break;
		}
		writeExecutableContent(stream, child, indent + 1);
		child = child.getNextSibling();
	}
	stream << padding << "}" << std::endl;
	stream << padding << ":: else -> ";

	if (nextIsElse) {
		child = condChain[1].getNextSibling();
		stream << "{" << std::endl;
		while(child) {
			writeExecutableContent(stream, child, indent + 1);
			child = child.getNextSibling();
		}
		stream << padding << "}" << std::endl;

	} else if (noNext) {
		stream << "skip;" << std::endl;
	} else {
		stream << "{" << std::endl;

		Arabica::XPath::NodeSet<std::string> cdrCondChain;
		for (int i = 1; i < condChain.size(); i++) {
			cdrCondChain.push_back(condChain[i]);
		}
		writeIfBlock(stream, cdrCondChain, indent + 1);
		stream << padding << "}" << std::endl;

	}

	stream << padding << "fi;" << std::endl;

}

std::string FSMToPromela::beautifyIndentation(const std::string& code, int indent) {

	std::string padding;
	for (int i = 0; i < indent; i++) {
		padding += "  ";
	}

	// remove topmost indentation from every line and reindent
	std::stringstream beautifiedSS;

	std::string initialIndent;
	bool gotIndent = false;
	bool isFirstLine = true;
	std::stringstream ssLine(code);
	std::string line;

	while(std::getline(ssLine, line)) {
		size_t firstChar = line.find_first_not_of(" \t\r\n");
		if (firstChar != std::string::npos) {
			if (!gotIndent) {
				initialIndent = line.substr(0, firstChar);
				gotIndent = true;
			}
			beautifiedSS << (isFirstLine ? "" : "\n") << padding << boost::replace_first_copy(line, initialIndent, "");
			isFirstLine = false;
		}
	}

	return beautifiedSS.str();
}

void FSMToPromela::writeDeclarations(std::ostream& stream) {

	// get all data elements
	NodeSet<std::string> datas = _xpath.evaluate("//" + _nsInfo.xpathPrefix + "data", _scxml).asNodeSet();
	NodeSet<std::string> dataText = filterChildType(Node_base::TEXT_NODE, datas, true);

	// write their text content
	stream << "// datamodel variables" << std::endl;
	for (int i = 0; i < dataText.size(); i++) {
		Node<std::string> data = dataText[i];
		stream << beautifyIndentation(data.getNodeValue()) << std::endl;
	}

	stream << std::endl;
	stream << "// global variables" << std::endl;
	stream << "int e;                   /* current event */" << std::endl;
	stream << "int s;                   /* current state */" << std::endl;
	stream << "chan iQ   = [100] of {int} /* internal queue */" << std::endl;
	stream << "chan eQ   = [100] of {int} /* external queue */" << std::endl;
	stream << "chan tmpQ = [100] of {int} /* temporary queue for external events in transitions */" << std::endl;
	stream << "int tmpQItem;" << std::endl;

	stream << std::endl;
	stream << "// event sources" << std::endl;

	if (_globalEventSource) {
		_globalEventSource.writeDeclarations(stream);
	}

	std::map<std::string, PromelaEventSource>::iterator invIter = _invokers.begin();
	while(invIter != _invokers.end()) {
		invIter->second.writeDeclarations(stream);
		invIter++;
	}

}

void FSMToPromela::writeEventSources(std::ostream& stream) {
	std::list<PromelaInline>::iterator inlineIter;

	if (_globalEventSource) {
		_globalEventSource.writeEventSource(stream);
	}

	std::map<std::string, PromelaEventSource>::iterator invIter = _invokers.begin();
	while(invIter != _invokers.end()) {
		invIter->second.writeEventSource(stream);
		invIter++;
	}

}


void FSMToPromela::writeFSM(std::ostream& stream) {
	NodeSet<std::string> transitions;

	stream << "proctype step() {" << std::endl;
	// write initial transition
	transitions = filterChildElements(_nsInfo.xmlNSPrefix + "transition", _startState);
	assert(transitions.size() == 1);
	stream << "  // transition's executable content" << std::endl;
	writeExecutableContent(stream, transitions[0], 1);

	for (int i = 0; i < _globalStates.size(); i++) {
		if (_globalStates[i] == _startState)
			continue;
		NodeSet<std::string> transitions = filterChildElements(_nsInfo.xmlNSPrefix + "transition", _globalStates[i]);
		for (int j = 0; j < transitions.size(); j++) {
			writeExecutableContent(stream, transitions[j], 1);
		}
	}

	stream << std::endl;
	stream << "nextStep:" << std::endl;
	stream << "  // push send events to external queue" << std::endl;
	stream << "  if" << std::endl;
	stream << "  :: len(tmpQ) != 0 -> { tmpQ?e; eQ!e }" << std::endl;
	stream << "  :: else -> skip;" << std::endl;
	stream << "  fi;" << std::endl << std::endl;

	stream << "  /* pop an event */" << std::endl;
	stream << "  if" << std::endl;
	stream << "  :: len(iQ) != 0 -> iQ ? e /* from internal queue */" << std::endl;
	stream << "  :: else -> eQ ? e         /* from external queue */" << std::endl;
	stream << "  fi;" << std::endl;
	stream << "  /* event dispatching per state */" << std::endl;
	stream << "  if" << std::endl;

	writeEventDispatching(stream);

	stream << "  :: else -> goto nextStep;" << std::endl;
	stream << "  fi;" << std::endl;
	stream << "terminate: skip;" << std::endl;

	// stop all event sources
	if (_globalEventSource)
		_globalEventSource.writeStopEventSources(stream, 1);

	std::map<std::string, PromelaEventSource>::iterator invIter = _invokers.begin();
	while(invIter != _invokers.end()) {
		invIter->second.writeStopEventSources(stream, 1);
		invIter++;
	}

	stream << "}" << std::endl;
}

void FSMToPromela::writeEventDispatching(std::ostream& stream) {
	for (int i = 0; i < _globalStates.size(); i++) {
		if (_globalStates[i] == _startState)
			continue;

		stream << "  :: (s == s" << i << ") -> {" << std::endl;

		NodeSet<std::string> transitions = filterChildElements(_nsInfo.xmlNSPrefix + "transition", _globalStates[i]);
		writeDispatchingBlock(stream, transitions, 2);
		stream << "    goto nextStep;" << std::endl;
		stream << "  }" << std::endl;
	}
}

void FSMToPromela::writeDispatchingBlock(std::ostream& stream, const Arabica::XPath::NodeSet<std::string>& transChain, int indent) {
	if (transChain.size() == 0)
		return;

	std::string padding;
	for (int i = 0; i < indent; i++) {
		padding += "  ";
	}

	stream << padding << "if" << std::endl;
	stream << padding << ":: ((0";

	Node<std::string> currTrans = transChain[0];
	std::string eventDesc = ATTR(currTrans, "event");
	if (boost::ends_with(eventDesc, "*"))
		eventDesc = eventDesc.substr(0, eventDesc.size() - 1);
	if (boost::ends_with(eventDesc, "."))
		eventDesc = eventDesc.substr(0, eventDesc.size() - 1);

	if (eventDesc.size() == 0) {
		stream << " || 1";
	} else {
		std::list<TrieNode*> trieNodes = _eventTrie.getWordsWithPrefix(eventDesc);

		std::list<TrieNode*>::iterator trieIter = trieNodes.begin();
		while(trieIter != trieNodes.end()) {
			stream << " || e == e" << (*trieIter)->identifier;
			trieIter++;
		}
	}

	stream << ") && ";
	stream << (HAS_ATTR(currTrans, "cond") ? ATTR(currTrans, "cond") : "1");
	stream << ") -> goto t" << _transitions[currTrans] << ";" << std::endl;
	;

	stream << padding << ":: else {" << std::endl;

	Arabica::XPath::NodeSet<std::string> cdrTransChain;
	for (int i = 1; i < transChain.size(); i++) {
		cdrTransChain.push_back(transChain[i]);
	}
	writeDispatchingBlock(stream, cdrTransChain, indent + 1);

	stream << padding << "  goto nextStep;" << std::endl;
	stream << padding << "}" << std::endl;
	stream << padding << "fi;" << std::endl;
}


void FSMToPromela::writeMain(std::ostream& stream) {
	stream << std::endl;
	stream << "init {" << std::endl;
	if (_globalEventSource)
		_globalEventSource.writeStartEventSources(stream, 1);
	stream << "  run step();" << std::endl;
	stream << "}" << std::endl;

}

void FSMToPromela::initNodes() {
	// get all states
	NodeSet<std::string> states = filterChildElements(_nsInfo.xmlNSPrefix + "state", _scxml);
	for (int i = 0; i < states.size(); i++) {
		_states[ATTR(states[i], "id")] = states[i];
		if (HAS_ATTR(states[i], "transient") && DOMUtils::attributeIsTrue(ATTR(states[i], "transient")))
			continue;
		_globalStates.push_back(states[i]);
	}
	_startState = _states[ATTR(_scxml, "initial")];

	// initialize event trie with all events that might occur
	NodeSet<std::string> internalEventNames;
	internalEventNames.push_back(_xpath.evaluate("//" + _nsInfo.xpathPrefix + "transition", _scxml).asNodeSet());
	internalEventNames.push_back(_xpath.evaluate("//" + _nsInfo.xpathPrefix + "raise", _scxml).asNodeSet());
	internalEventNames.push_back(_xpath.evaluate("//" + _nsInfo.xpathPrefix + "send", _scxml).asNodeSet());

	for (int i = 0; i < internalEventNames.size(); i++) {
		if (HAS_ATTR(internalEventNames[i], "event")) {
			std::string eventNames = ATTR(internalEventNames[i], "event");
			std::list<std::string> events = tokenizeIdRefs(eventNames);
			for (std::list<std::string>::iterator eventIter = events.begin();
			        eventIter != events.end(); eventIter++) {
				std::string eventName = *eventIter;
				if (boost::ends_with(eventName, "*"))
					eventName = eventName.substr(0, eventName.size() - 1);
				if (boost::ends_with(eventName, "."))
					eventName = eventName.substr(0, eventName.size() - 1);
				_eventTrie.addWord(eventName);
			}
		}
	}

	// external event names from comments
	NodeSet<std::string> promelaEventSourceComments;
	NodeSet<std::string> invokers = _xpath.evaluate("//" + _nsInfo.xpathPrefix + "invoke", _scxml).asNodeSet();
	promelaEventSourceComments.push_back(filterChildType(Node_base::COMMENT_NODE, invokers, false)); // comments in invoke elements
	promelaEventSourceComments.push_back(filterChildType(Node_base::COMMENT_NODE, _scxml, false)); // comments in scxml element

	for (int i = 0; i < promelaEventSourceComments.size(); i++) {
		PromelaInlines promInls = getInlinePromela(promelaEventSourceComments[i]);
		PromelaEventSource promES(promInls, promelaEventSourceComments[i].getParentNode());

		if (TAGNAME(promelaEventSourceComments[i].getParentNode()) == "scxml") {
			promES.type = PromelaEventSource::PROMELA_EVENT_SOURCE_GLOBAL;
			promES.trie = &_eventTrie;
			promES.name = "global";
			_globalEventSource = promES;
		} else if (TAGNAME(promelaEventSourceComments[i].getParentNode()) == "invoke") {
			if (!HAS_ATTR(promelaEventSourceComments[i].getParentNode(), "invokeid")) {
				Element<std::string> invoker = Element<std::string>(promelaEventSourceComments[i].getParentNode());
				invoker.setAttribute("invokeid", "invoker" + toStr(_invokers.size()));
			}
			std::string invokeId = ATTR(promelaEventSourceComments[i].getParentNode(), "invokeid");
			promES.type = PromelaEventSource::PROMELA_EVENT_SOURCE_INVOKER;
			promES.trie = &_eventTrie;
			promES.name = invokeId;
			_invokers[invokeId] = promES;
		}
	}

	// enumerate transitions
	NodeSet<std::string> transitions = filterChildElements(_nsInfo.xmlNSPrefix + "transition", _scxml, true);
	int index = 0;
	for (int i = 0; i < transitions.size(); i++) {
		_transitions[transitions[i]] = index++;
	}
}

void PromelaInline::dump() {
	std::list<std::list<std::string> >::iterator outerIter = sequences.begin();
	while(outerIter != sequences.end()) {
		std::list<std::string>::iterator innerIter = outerIter->begin();
		while(innerIter != outerIter->end()) {
			std::cout << *innerIter << " ";
			innerIter++;
		}
		std::cout << std::endl;
		outerIter++;
	}
}

void FSMToPromela::writeProgram(std::ostream& stream) {

	if (!HAS_ATTR(_scxml, "flat") || !DOMUtils::attributeIsTrue(ATTR(_scxml, "flat"))) {
		LOG(ERROR) << "Given SCXML document was not flattened";
		return;
	}

	if (!HAS_ATTR(_scxml, "datamodel") || ATTR(_scxml, "datamodel") != "promela") {
		LOG(ERROR) << "Can only convert SCXML documents with \"promela\" datamodel";
		return;
	}

	if (HAS_ATTR(_scxml, "binding") && ATTR(_scxml, "binding") != "early") {
		LOG(ERROR) << "Can only convert for early data bindings";
		return;
	}

	initNodes();

	writeEvents(stream);
	stream << std::endl;
	writeStates(stream);
	stream << std::endl;
	writeDeclarations(stream);
	stream << std::endl;
	writeEventSources(stream);
	stream << std::endl;
	writeFSM(stream);
	stream << std::endl;
	writeMain(stream);
	stream << std::endl;

}

}
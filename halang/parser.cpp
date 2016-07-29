
#include "stdafx.h"
#include "parser.h"
#include "ast.h"

#define CHECK_NULL(PTR) if ((PTR) == nullptr) return nullptr;

namespace parser
{

	Parser::Parser(Lexer& _lex) : 
		lexer(_lex), ok(true), ast_root(nullptr)
	{
		nextToken();
	}

	Parser::Parser(Parser&& _parser) :
		_MessageContainer(_parser),
		lexer(_parser.lexer), ok(_parser.ok), 
		ast_root(move(_parser.ast_root))
	{ }

	void Parser::parse()
	{
		this->ast_root = this->parseChunk();
	}

	Token Parser::nextToken()
	{
		Token tmp = move(lookahead);
		lookahead = lexer.read();
		return tmp;
	}

	/**********************************/
	/*   Paring                       */
	/**********************************/

	// chunk ::= block
	Node* Parser::parseChunk()
	{
		return parseBlock();
	}


	// block ::= { stat }
	// First(block) = First(stat)
	Node* Parser::parseBlock()
	{
		auto _block = make_node<BlockExprNode>();
		
		do
		{
			if (match(Token::TYPE::SEMICOLON))
				nextToken();
			else
				_block->children.push_back(parseStatement());
		} while (match(Token::TYPE::IDENTIFIER) ||
			match(Token::TYPE::NUMBER) ||
			match(Token::TYPE::VAR) ||
			match(Token::TYPE::OPEN_PAREN) ||
			match(Token::TYPE::WHILE) ||
			match(Token::TYPE::IF) ||
			match(Token::TYPE::FUNC) ||
			match(Token::TYPE::SEMICOLON) ||
			Token::isOperator(lookahead)
			);

		return _block;
	}

	Node* Parser::parseStatement()
	{
		if (match(Token::TYPE::SEMICOLON))
		{
			nextToken();
			return nullptr;
		}
		else if (match(Token::TYPE::VAR))
			return parseVarStmt();
		else if (match(Token::TYPE::WHILE))
			return parseWhileStmt();
		else if (match(Token::TYPE::BREAK))
		{
			nextToken();
			return make_node<BreakStmtNode>();
		}
		else if (match(Token::TYPE::IF))
			return parseIfStmt();
		else if (match(Token::TYPE::FUNC))
			return parseFuncDef();
		else if (match(Token::TYPE::RETURN))
			return parseReturnStmt();
		else if (match(Token::TYPE::OPEN_BRAKET))
		{
			expect(nextToken(), Token::TYPE::OPEN_BRAKET);
			auto _node = parseBlock();
			expect(nextToken(), Token::TYPE::CLOSE_BRAKET);
			return _node;
		}
		else
			return parseExpression();
	}

	// varStmt ::= VAR ID '=' exp [ , ID '=' EXP ]
	Node* Parser::parseVarStmt()
	{
		expect(nextToken(), Token::TYPE::VAR);
		Node *exp = nullptr;
		AssignmentNode* _assign = nullptr;
		auto _var = make_node<VarStmtNode>();
		if (match(Token::TYPE::IDENTIFIER))
		{
			Token _tk = nextToken();
			expect(nextToken(), Token::TYPE::ASSIGN);
			exp = parseStatement();

			auto _assign = make_node<AssignmentNode>(make_node<IdentifierNode>(*_tk._literal), exp);

			_var->children.push_back(_assign);

			while (match(Token::TYPE::COMMA))
			{
				nextToken();
				expect(Token::TYPE::IDENTIFIER);
				Token _tk = nextToken();
				expect(nextToken(), Token::TYPE::ASSIGN);
				exp = parseStatement();

				_assign = make_node<AssignmentNode>(make_node<IdentifierNode>(*_tk._literal), exp);
				_var->children.push_back(_assign);
			}
		}
		else
		{
			ReportError("<Var stmt> expected identifier");
		}
		return _var;
	}

	Node* Parser::parseExpression()
	{
		// Expression ::=
		//		Assignment | BinaryExpr
		// 
		// Assignment ::= Variable '=' Expression
		//
		// BinaryExpr :: BinaryExpr OP BinaryExpr | UnaryExpr
		//
		// Variable ::= ID ( '[' Expression ']' )?
		//
		// UnaryExpr ::= ('+' | '-' | '!') UnaryExpr | '(' Expression ')' | FuncCall
		//
		// FuncCall ::= ID '(' FuncCallArgs ')'
		if (match(Token::TYPE::IDENTIFIER))
		{
			Token _tk = nextToken();
			auto _id = make_node<IdentifierNode>(*_tk._literal);
			if (match(Token::TYPE::OPEN_PAREN))
			{
				auto _func = parseFuncCall(_id);
				if (Token::isOperator(lookahead))
				{
					auto _op = nextToken();
					return parseBinaryExpr(_func, _op);
				}
				else
					return _func;
			}
			else if (match(Token::TYPE::ASSIGN))
				return parseAssignment(_id);
			else if (Token::isOperator(lookahead))
			{
				auto _tk = nextToken();
				return parseBinaryExpr(_id, _tk);
			}
			else return _id;
		}
		else
			return parseBinaryExpr();
	}

	Node* Parser::parseVarExpr()
	{
		expect(Token::TYPE::VAR);
		nextToken();
		return parseAssignment();
	}

	Node* Parser::parseAssignment(IdentifierNode* _id)
	{
		// current version:
		// Assignment ::=
		//		VAR '=' Expression
		//
		// future version:
		// Assignment ::=
		//		VAR { ',' VAR } = Expression { ',' Expression }
		//
		if (!_id)
		{
			expect(Token::TYPE::IDENTIFIER);
			string _str = *lookahead._literal;
			nextToken();
			_id = make_node<IdentifierNode>(_str);
		}

		expect(nextToken(), Token::TYPE::ASSIGN);
		Node* _exp = parseExpression();
		return make_node<AssignmentNode>(_id, _exp);
	}

	Node* Parser::parseUnaryExpr(OperatorType _op)
	{
		// UnaryExpr ::= 
		//	( '+' | '-' | '!' ) UnaryExpr | Number | VAR | FUNCTIONCALL
		Node* _node = nullptr;
		if (match(Token::TYPE::ADD) || match(Token::TYPE::SUB) || match(Token::TYPE::NOT))
		{
			Token _op = nextToken();
			_node = make_node<UnaryExprNode>(Token::toOperator(_op), parseUnaryExpr());
		}
		else if (match(Token::TYPE::NUMBER))
		{
			_node = make_node<NumberNode>(lookahead._double, lookahead.maybeInt);
			nextToken();
		}
		else if (match(Token::TYPE::IDENTIFIER)) // maybe a variable or functioncall
		{
			std::string _id_name = *nextToken()._literal;
			_node = make_node<IdentifierNode>(_id_name);

			if (match(Token::TYPE::OPEN_PAREN))
				_node = parseFuncCall(_node);
		} 
		else if (match(Token::TYPE::OPEN_PAREN))
		{
			nextToken();
			auto exp = parseExpression();
			expect(nextToken(), Token::TYPE::CLOSE_PAREN);
			if (match(Token::TYPE::OPEN_PAREN))
				exp = parseFuncCall(exp);
			return exp;
		}
		return _node;
	}

	Node* Parser::parseBinaryExpr(Node* left_exp, Token left_tk)
	{
		Node* exp = parseUnaryExpr();
		CHECK_NULL(exp);
		OperatorType left_op = Token::toOperator(left_tk);
		while (true)
		{
			Token right_tk = lookahead;
			OperatorType right_op = Token::toOperator(right_tk);
			if (getPrecedence(left_op) < getPrecedence(right_op))
			{
				nextToken();
				exp = parseBinaryExpr(exp, right_tk);
			}
			else if (getPrecedence(left_op) == getPrecedence(right_op))
			{
				if (getPrecedence(left_op) == 0)
					return exp;
				CHECK_NULL(left_exp);
				exp = make_node<BinaryExprNode>(left_op, left_exp, exp);
				nextToken();
				return parseBinaryExpr(exp, right_tk);
			}
			else // left_op > right_op
			{
				if (left_exp)
					exp = make_node<BinaryExprNode>(left_op, left_exp, exp);
				return exp;
			}
		}
	}

	Node* Parser::parseIfStmt()
	{
		expect(nextToken(), Token::TYPE::IF);
		expect(nextToken(), Token::TYPE::OPEN_PAREN);
		auto _condition = parseBinaryExpr();
		expect(nextToken(), Token::TYPE::CLOSE_PAREN);
		expect(nextToken(), Token::TYPE::OPEN_BRAKET);
		auto _block = parseBlock();
		expect(nextToken(), Token::TYPE::CLOSE_BRAKET);
		Node* _else = nullptr;
		if (match(Token::TYPE::ELSE))
			_else = parseElseStmt();
		return make_node<IfStmtNode>(_condition, _block, _else);
	}

	Node* Parser::parseElseStmt()
	{
		expect(nextToken(), Token::TYPE::ELSE);
		if (match(Token::TYPE::IF))
			return parseIfStmt();
		expect(nextToken(), Token::TYPE::OPEN_BRAKET);
		auto _block = parseBlock();
		expect(nextToken(), Token::TYPE::CLOSE_BRAKET);
		return _block;
	}

	Node* Parser::parseWhileStmt()
	{
		expect(nextToken(), Token::TYPE::WHILE);
		expect(nextToken(), Token::TYPE::OPEN_PAREN); // (
		auto _condition = parseBinaryExpr();
		expect(nextToken(), Token::TYPE::CLOSE_PAREN);
		expect(nextToken(), Token::TYPE::OPEN_BRAKET);
		auto _block = parseBlock();
		expect(nextToken(), Token::TYPE::CLOSE_BRAKET);
		return make_node<WhileStmtNode>(_condition, _block);
	}

	Node* Parser::parseFuncDef()
	{
		// def new function
		auto _func = make_node<FuncDefNode>();
		expect(nextToken(), Token::TYPE::FUNC);
		expect(Token::TYPE::IDENTIFIER);
		_func->name = *nextToken()._literal;
		expect(nextToken(), Token::TYPE::OPEN_PAREN);
		_func->parameters = dynamic_cast<FuncDefParamsNode*>(parseFuncDefParams());
		expect(nextToken(), Token::TYPE::CLOSE_PAREN);
		expect(nextToken(), Token::TYPE::OPEN_BRAKET);
		_func->block = dynamic_cast<BlockExprNode*>(parseBlock());
		expect(nextToken(), Token::TYPE::CLOSE_BRAKET);
		return _func;
	}

	Node* Parser::parseFuncDefParams()
	{
		auto _params = make_node<FuncDefParamsNode>();
		expect(Token::TYPE::IDENTIFIER);
		while (match(Token::TYPE::IDENTIFIER))
		{
			Token t = nextToken();
			_params->identifiers.push_back(*t._literal);
			if (match(Token::TYPE::CLOSE_PAREN))
				break;
			else if (match(Token::COMMA))
				nextToken();
			else
			{
				ReportError("Unexpected token.");
				break;
			}
		}
		return _params;
	}

	Node* Parser::parseFuncCall(Node* exp)
	{
		FuncCallNode* _node = nullptr;
		Node * _params = nullptr;
		expect(nextToken(), Token::TYPE::OPEN_PAREN);
		_params = parseFuncCallParams();
		CHECK_NULL(_params);
		expect(nextToken(), Token::TYPE::CLOSE_PAREN);
		_node = make_node<FuncCallNode>(exp, dynamic_cast<FuncCallParamsNode*>(_params));
		while (match(Token::TYPE::OPEN_PAREN))
		{
			nextToken();
			_params = parseFuncCallParams();
			expect(nextToken(), Token::TYPE::CLOSE_PAREN);
			_node = make_node<FuncCallNode>(_node, dynamic_cast<FuncCallParamsNode*>(_params));
		}
		return _node;
	}

	Node* Parser::parseFuncCallParams()
	{
		auto _params = make_node<FuncCallParamsNode>();
		if (!match(Token::TYPE::CLOSE_PAREN))
		{
			auto _exp = parseExpression();
			_params->children.push_back(_exp);
			while (match(Token::TYPE::COMMA))
			{
				nextToken();
				_exp = parseExpression();
				_params->children.push_back(_exp);
			}
		}
		return _params;
	}

	Node* Parser::parseReturnStmt()
	{
		return nullptr;
	}

	Parser::~Parser()
	{
		for (auto i = _node_list.begin(); i != _node_list.end(); ++i)
			delete *i;
	}

}

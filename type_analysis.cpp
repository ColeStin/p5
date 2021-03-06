#include "ast.hpp"
#include "symbol_table.hpp"
#include "errors.hpp"
#include "types.hpp"
#include "name_analysis.hpp"
#include "type_analysis.hpp"

namespace cminusminus{

TypeAnalysis * TypeAnalysis::build(NameAnalysis * nameAnalysis){
	//To emphasize that type analysis depends on name analysis
	// being complete, a name analysis must be supplied for 
	// type analysis to be performed.
	TypeAnalysis * typeAnalysis = new TypeAnalysis();
	auto ast = nameAnalysis->ast;	
	typeAnalysis->ast = ast;

	ast->typeAnalysis(typeAnalysis);
	if (typeAnalysis->hasError){
		return nullptr;
	}

	return typeAnalysis;

}

void ProgramNode::typeAnalysis(TypeAnalysis * ta){

	//pass the TypeAnalysis down throughout
	// the entire tree, getting the types for
	// each element in turn and adding them
	// to the ta object's hashMap
	for (auto global : *myGlobals){
		global->typeAnalysis(ta);
	}

	//The type of the program node will never
	// be needed. We can just set it to VOID
	//(Alternatively, we could make our type 
	// be error if the DeclListNode is an error)
	ta->nodeType(this, BasicType::produce(VOID));
}

void AssignStmtNode::typeAnalysis(TypeAnalysis * ta){
	myExp->typeAnalysis(ta);

	//It can be a bit of a pain to write 
	// "const DataType *" everywhere, so here
	// the use of auto is used instead to tell the
	// compiler to figure out what the subType variable
	// should be
	auto subType = ta->nodeType(myExp);

	// As error returns null if subType is NOT an error type
	// otherwise, it returns the subType itself
	if (subType->asError()){
		ta->nodeType(this, subType);
	} else {
		ta->nodeType(this, BasicType::produce(VOID));
	}
}

void PostDecStmtNode::typeAnalysis(TypeAnalysis * ta){

	myLVal->typeAnalysis(ta);
}

void PostIncStmtNode::typeAnalysis(TypeAnalysis * ta){

	myLVal->typeAnalysis(ta);
}

void ReadStmtNode::typeAnalysis(TypeAnalysis * ta){

    myDst->typeAnalysis(ta);
    auto subType = ta->nodeType(myDst);
    if (subType->asFn()){
        ta->errReadFn(myDst->pos());
    }
}

void WriteStmtNode::typeAnalysis(TypeAnalysis * ta){

    mySrc->typeAnalysis(ta);

    auto subType = ta->nodeType(mySrc);
    if(subType->asFn()){
        if(subType->isVoid()){

            ta->errWriteVoid(mySrc->pos());
            auto fn = subType->asFn();
        }
        else{

            ta->errWriteFn(mySrc->pos());
               auto fn = subType->asFn();
        }
    }
}

void IfStmtNode::typeAnalysis(TypeAnalysis * ta){

	myCond->typeAnalysis(ta);
	for (auto stmt : *myBody){
		stmt->typeAnalysis(ta);
	}

	auto condition = ta->nodeType(myCond);
	if(!condition->isBool()){
		ta->errIfCond(myCond->pos());
	}
	else{
		ta->nodeType(this, BasicType::produce(VOID));
	}
	ta->nodeType(this, ErrorType::produce());
}

void IfElseStmtNode::typeAnalysis(TypeAnalysis * ta){

	myCond->typeAnalysis(ta);
	for (auto stmt : *myBodyTrue){
		stmt->typeAnalysis(ta);
	}
	for (auto stmt : *myBodyFalse){
		stmt->typeAnalysis(ta);
	}

	auto condition = ta->nodeType(myCond);
	if(!condition->isBool()){
		ta->errIfCond(myCond->pos());
	}
	else{
		ta->nodeType(this, BasicType::produce(VOID));
	}
	ta->nodeType(this, ErrorType::produce());
}

void WhileStmtNode::typeAnalysis(TypeAnalysis * ta){

	myCond->typeAnalysis(ta);
	for (auto stmt : *myBody){
		stmt->typeAnalysis(ta);
	}

	auto condition = ta->nodeType(myCond);
	if(!condition->isBool()){
		ta->errIfCond(myCond->pos());
	}
	else{
		ta->nodeType(this, BasicType::produce(VOID));
	}
	ta->nodeType(this, ErrorType::produce());
}

void VarDeclNode::typeAnalysis(TypeAnalysis * ta){
	// VarDecls always pass type analysis, since they 
	// are never used in an expression. You may choose
	// to type them void (like this), as discussed in class
	ta->nodeType(this, BasicType::produce(VOID));
}

void FnDeclNode::typeAnalysis(TypeAnalysis * ta){
	//std::cout<<"funciton declaration\n";
	//HINT: you might want to change the signature for
	// typeAnalysis on FnBodyNode to take a second
	// argument which is the type of the current 
	// function. This will help you to know at a 
	// return statement whether the return type matches
	// the current function

	//Note: this function may need extra code

	// if(myRetType->getType()->isVoid()){
	// 	std::cout<<"void function\n";
	// }
	//BasicType::produce(VOID)


	//i "borrowed" ur code, sorry. I tried to reinvent it but this was just too good.
	std::list<const DataType *> * formalTypes = 
		new std::list<const DataType *>();
	for (auto formal : *(this->myFormals)){
		formal->typeAnalysis(ta);
		TypeNode * typeNode = formal->getTypeNode();
		const DataType * formalType = typeNode->getType();
		formalTypes->push_back(formalType);
	}
	const DataType * retType = this->getRetTypeNode()->getType();
	FnType * functionType = new FnType(formalTypes, retType);
	ta->setCurrentFnType(functionType);
	
	for (auto stmt : *myBody){
		stmt->typeAnalysis(ta);
	}
	ta->nodeType(myID, functionType);
	
}

void BinaryExpNode::typeAnalysis(TypeAnalysis * ta){
	//should be overriden by subclass
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2){
		ta->errMathOpd(myExp2->pos());
	}else{
		ta->nodeType(this, exp1);
		return;
	}
	ta->nodeType(this, ErrorType::produce());
}

void PlusNode::typeAnalysis(TypeAnalysis * ta){

	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || !exp1->isInt()){
		ta->errMathOpd(myExp2->pos());
	}else{
		ta->nodeType(this, exp1);
		return;
	}
	ta->nodeType(this, ErrorType::produce());
}

void MinusNode::typeAnalysis(TypeAnalysis * ta){

	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || !exp1->isInt()){
		ta->errMathOpd(myExp2->pos());
	}else{
		ta->nodeType(this, exp1);
		return;
	}
	ta->nodeType(this, ErrorType::produce());
}

void DivideNode::typeAnalysis(TypeAnalysis * ta){

	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || !exp1->isInt()){
		ta->errMathOpd(myExp2->pos());
	}else{
		ta->nodeType(this, exp1);
		return;
	}
	ta->nodeType(this, ErrorType::produce());
}

void TimesNode::typeAnalysis(TypeAnalysis * ta){

	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || !exp1->isInt()){
		ta->errMathOpd(myExp2->pos());
	}else{
		ta->nodeType(this, exp1);
		return;
	}
	ta->nodeType(this, ErrorType::produce());
}

void AndNode::typeAnalysis(TypeAnalysis * ta){

	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || !exp1->isBool()){
		ta->errMathOpd(myExp2->pos());
	}else{
		ta->nodeType(this, BasicType::produce(BOOL));
		return;
	}
	ta->nodeType(this, ErrorType::produce());

}

void OrNode::typeAnalysis(TypeAnalysis * ta){

	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || !exp1->isBool()){
		ta->errMathOpd(myExp2->pos());
	}else{
		ta->nodeType(this, BasicType::produce(BOOL));
		return;
	}
	ta->nodeType(this, ErrorType::produce());
	
}

void EqualsNode::typeAnalysis(TypeAnalysis * ta){

	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2){
		ta->errEqOpr(myExp2->pos());
	}else{
		ta->nodeType(this, BasicType::produce(BOOL));
		return;
	}
	ta->nodeType(this, ErrorType::produce());

	
}

void NotEqualsNode::typeAnalysis(TypeAnalysis * ta){

	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2){
		ta->errEqOpr(myExp2->pos());
	}else{
		ta->nodeType(this, BasicType::produce(BOOL));
		return;
	}
	ta->nodeType(this, ErrorType::produce());


}

void LessEqNode::typeAnalysis(TypeAnalysis * ta){

	
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || exp1->isString() || exp1->isVoid()){
		ta->errRelOpd(myExp2->pos());
	}else{
		ta->nodeType(this, BasicType::produce(BOOL));
		return;
	}
	ta->nodeType(this, ErrorType::produce());


}

void LessNode::typeAnalysis(TypeAnalysis * ta){

	
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || exp1->isString() || exp1->isVoid()){
		ta->errRelOpd(myExp2->pos());
	}else{
		ta->nodeType(this, BasicType::produce(BOOL));
		return;
	}
	ta->nodeType(this, ErrorType::produce());

	
}

void GreaterEqNode::typeAnalysis(TypeAnalysis * ta){

	
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || exp1->isString() || exp1->isVoid()){
		ta->errRelOpd(myExp2->pos());
	}else{
		ta->nodeType(this, BasicType::produce(BOOL));
		return;
	}
	ta->nodeType(this, ErrorType::produce());

	
}

void GreaterNode::typeAnalysis(TypeAnalysis * ta){

	
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto exp1 = ta->nodeType(myExp1);
	auto exp2 = ta->nodeType(myExp2);

	if(exp1 != exp2 || exp1->isString() || exp1->isVoid()){
		ta->errRelOpd(myExp2->pos());
	}else{
		ta->nodeType(this, BasicType::produce(BOOL));
		return;
	}
	ta->nodeType(this, ErrorType::produce());


}

void CallExpNode::typeAnalysis(TypeAnalysis * ta){
	myID->typeAnalysis(ta);
    auto subType = ta->nodeType(myID)->asFn();
	if(subType == nullptr){
		std::cout<<"you did not call a function\n";
	}else{
		int funcArgSize = 0;
		int callArgSize = 0;
		auto funcTypes = subType->getFormalTypes();
		std::list<const DataType *> * callTypes = 
			new std::list<const DataType *>();
		for (auto formal : *(this->myArgs)){
			callArgSize++;
			formal->typeAnalysis(ta);
			auto exp = ta->nodeType(formal);
			callTypes->push_back(exp);
		}
		for(auto funcArg: * funcTypes){
			funcArgSize++;
		}
		//std::cout<<"Call Arg Size: "<<callArgSize<<"  | Func Arg Size: "<<funcArgSize<<std::endl;
		if(funcArgSize == callArgSize){
			//need to step through list, it doesnt have random access iterator
			auto funcFront = funcTypes->begin();
			auto callFront = callTypes->begin();
			bool correctArgs = true;
			for(int size = 0; size<funcArgSize; size++){
				cout<<size<<". ";
				if(* funcFront != * callFront){
					std::cout<<"mismatched call argument types";
					correctArgs = false;
				}
				advance(funcFront, 1);
				advance(callFront, 1);
				std::cout<<"\n";
				
			}
			if(correctArgs == true){
				// need to set this node to the same type as the return type
				// tempted to just biff it and use a if-elif-else
				// ta->nodeType(this, BasicType::produce(subType->getReturnType()))
			}
		}
		else{
			std::cout<<"call does not have correct number of arguments\n";
		}
		
	}
	ta->nodeType(this, ErrorType::produce());

}

void RefNode::typeAnalysis(TypeAnalysis * ta){

	myExp->typeAnalysis(ta);
}

void DerefNode::typeAnalysis(TypeAnalysis * ta){

	myID->typeAnalysis(ta);
}

void NegNode::typeAnalysis(TypeAnalysis * ta){

	myExp->typeAnalysis(ta);
}

void NotNode::typeAnalysis(TypeAnalysis * ta){

	myExp->typeAnalysis(ta);
}

void AssignExpNode::typeAnalysis(TypeAnalysis * ta){
	//TODO: Note that this function is incomplete. 
	// and needs additional code

	//Do typeAnalysis on the subexpressions
	myDst->typeAnalysis(ta);
	mySrc->typeAnalysis(ta);

	const DataType * tgtType = ta->nodeType(myDst);
	const DataType * srcType = ta->nodeType(mySrc);

	//While incomplete, this gives you one case for 
	// assignment: if the types are exactly the same
	// it is usually ok to do the assignment. One
	// exception is that if both types are function
	// names, it should fail type analysis
	if (tgtType == srcType){
		ta->nodeType(this, tgtType);
		return;
	}
	
	//Some functions are already defined for you to
	// report type errors. Note that these functions
	// also tell the typeAnalysis object that the
	// analysis has failed, meaning that main.cpp
	// will print "Type check failed" at the end
	ta->errAssignOpr(this->pos());


	//Note that reporting an error does not set the
	// type of the current node, so setting the node
	// type must be done
	ta->nodeType(this, ErrorType::produce());
}

void ReturnStmtNode::typeAnalysis(TypeAnalysis * ta){
	//check type of current  function\check if expression is that type
	//std::cout<<"return\n";
	if(myExp != nullptr){
		myExp->typeAnalysis(ta);
		auto subType = ta->nodeType(myExp);
		if(ta->getCurrentFnType()->getReturnType()->validVarType()){
			ta->nodeType(this, subType);
			return;
		}
		else{
			std::cout<<"void returning value\n";
		}
	}
	else{
		if(ta->getCurrentFnType()->getReturnType()->validVarType()){
			std::cout<<"throw a missing return value error\n";
		}else{
			ta->nodeType(this, BasicType::produce(VOID));
			return;
		}
	}
	ta->nodeType(this, ErrorType::produce());
	
}

void CallStmtNode::typeAnalysis(TypeAnalysis * ta){
	myCallExp->typeAnalysis(ta);
	ta->nodeType(this, ErrorType::produce());
}

void TypeNode::typeAnalysis(TypeAnalysis * ta){

	//work in progress: placeholder code
}

void IntLitNode::typeAnalysis(TypeAnalysis * ta){
	// IntLits never fail their type analysis and always
	// yield the type INT
	ta->nodeType(this, BasicType::produce(INT));
}

void ShortLitNode::typeAnalysis(TypeAnalysis * ta){
	// IntLits never fail their type analysis and always
	// yield the type INT
	ta->nodeType(this, BasicType::produce(SHORT));
}

void StrLitNode::typeAnalysis(TypeAnalysis * ta){
	// IntLits never fail their type analysis and always
	// yield the type INT
	ta->nodeType(this, BasicType::produce(STRING));
}

void TrueNode::typeAnalysis(TypeAnalysis * ta){
	// IntLits never fail their type analysis and always
	// yield the type INT
	ta->nodeType(this, BasicType::produce(BOOL));
}

void FalseNode::typeAnalysis(TypeAnalysis * ta){
	// IntLits never fail their type analysis and always
	// yield the type INT
	ta->nodeType(this, BasicType::produce(BOOL));
}

void IDNode::typeAnalysis(TypeAnalysis * ta){
	// IDs never fail type analysis and always
	// yield the type of their symbol (which
	// depends on their definition)
	ta->nodeType(this, this->getSymbol()->getDataType());
}



/*
	everything past this point has no entry in the nameAnalysis program.
	They're just here for the moment because I didn't want to remove them
	yet in case it messed something up.
*/

void StmtNode::typeAnalysis(TypeAnalysis * ta){

	//work in progress: placeholder code
}

void ExpNode::typeAnalysis(TypeAnalysis * ta){
	TODO("Override me in the subclass");
}

void UnaryExpNode::typeAnalysis(TypeAnalysis * ta){

	//work in progress: placeholder code
}

void DeclNode::typeAnalysis(TypeAnalysis * ta){
	TODO("Override me in the subclass");
}

}

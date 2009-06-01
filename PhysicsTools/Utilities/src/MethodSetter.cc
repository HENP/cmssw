#include "PhysicsTools/Utilities/src/MethodSetter.h"
#include "PhysicsTools/Utilities/src/returnType.h"
#include "PhysicsTools/Utilities/src/findMethod.h"
#include "PhysicsTools/Utilities/src/findDataMember.h"
#include "PhysicsTools/Utilities/src/ErrorCodes.h"
#include "PhysicsTools/Utilities/interface/Exception.h"
#include <string>
using namespace reco::parser;
using namespace std;
using namespace ROOT::Reflex;

void MethodSetter::operator()(const char * begin, const char * end) const {
  string name(begin, end);
  string::size_type parenthesis = name.find_first_of('(');
  std::vector<AnyMethodArgument> args;
  if(parenthesis != string::npos) {
    name.erase(parenthesis, name.size());
    if(intStack_.size()==0)
      throw Exception(begin)
	<< "expected method argument, but non given.";    
    for(vector<AnyMethodArgument>::const_iterator i = intStack_.begin(); i != intStack_.end(); ++i)
      args.push_back(*i);
    intStack_.clear();
  }
  string::size_type endOfExpr = name.find_last_of(' ');
  if(endOfExpr != string::npos)
    name.erase(endOfExpr, name.size());
  //std::cout << "\nPushing '" << name << "', #args = " << args.size() << ", begin=[" << begin << "]" << std::endl; 
  push(name, args,begin);
}

void MethodSetter::push(const string & name, const vector<AnyMethodArgument> & args, const char* begin) const {
  Type type = typeStack_.back();
  vector<AnyMethodArgument> fixups;
  int error;
  pair<Member, bool> mem = reco::findMethod(type, name, args, fixups,begin,error);
  if(mem.first) {
     Type retType = reco::returnType(mem.first);
     if(!retType) {
        throw Exception(begin)
     	<< "member \"" << mem.first.Name() << "\" return type is ivalid: \"" 
     	<<  mem.first.TypeOf().Name() << "\"";
        
     }
     typeStack_.push_back(retType);
     // check for edm::Ref, edm::RefToBase, edm::Ptr
     if(mem.second) {
        //std::cout << "Mem.second, so book " << mem.first.Name() << " without fixups." << std::endl;
        methStack_.push_back(MethodInvoker(mem.first));
        push(name, args,begin); // we have not found the method, so we have not fixupped the arguments
      } else {
        //std::cout << "Not mem.second, so book " << mem.first.Name() << " with #args = " << fixups.size() << std::endl;
        methStack_.push_back(MethodInvoker(mem.first, fixups));
      }
  } else {
     if(error != reco::parser::kNameDoesNotExist) {
        switch(error) {
           case reco::parser::kIsNotPublic:
            throw Exception(begin)
              << "method named \"" << name << "\" for type \"" 
              <<type.Name() << "\" is not publically accessible.";
            break;
           case reco::parser::kIsStatic:
             throw Exception(begin)
               << "method named \"" << name << "\" for type \"" 
               <<type.Name() << "\" is static.";
             break;
           case reco::parser::kIsNotConst:
              throw Exception(begin)
                << "method named \"" << name << "\" for type \"" 
                <<type.Name() << "\" is not const.";
              break;
           case reco::parser::kWrongNumberOfArguments:
               throw Exception(begin)
                 << "method named \"" << name << "\" for type \"" 
                 <<type.Name() << "\" was passed the wrong number of arguments.";
               break;
           case reco::parser::kWrongArgumentType:
               throw Exception(begin)
                     << "method named \"" << name << "\" for type \"" 
                     <<type.Name() << "\" was passed the wrong types of arguments.";
               break;
           default:  
            throw Exception(begin)
             << "method named \"" << name << "\" for type \"" 
             <<type.Name() << "\" is not usable in this context.";
        }
     }
     //see if it is a member data
     int error;
     Member member = reco::findDataMember(type,name,error);
     if(!member) {
        switch(error) {
           case reco::parser::kNameDoesNotExist:
            throw Exception(begin)
               << "no method or data member named \"" << name << "\" found for type \"" 
               <<type.Name() << "\"";
            break;
           case reco::parser::kIsNotPublic:
            throw Exception(begin)
              << "data member named \"" << name << "\" for type \"" 
              <<type.Name() << "\" is not publically accessible.";
            break;
           default:
           throw Exception(begin)
             << "data member named \"" << name << "\" for type \"" 
             <<type.Name() << "\" is not usable in this context.";
           break;
        }
     }
     typeStack_.push_back(member.TypeOf());
     methStack_.push_back(MethodInvoker(member));
  }
}
    

#ifndef SHARED_H
#define SHARED_H

#include <kjs/shared_ptr.h>

namespace khtml {

template<class type> class Shared
{
public:
    Shared() { _ref=0; /*counter++;*/ }
    ~Shared() { /*counter--;*/ }

    void ref() { _ref++;  }
    void deref() { 
	if(_ref) _ref--; 
	if(!_ref)
	    delete static_cast<type *>(this); 
    }
    bool hasOneRef() { //kdDebug(300) << "ref=" << _ref << endl;
    	return _ref==1; }

    int refCount() const { return _ref; }
//    static int counter;
protected:
    unsigned int _ref;

private:
    Shared(const Shared &);
    Shared &operator=(const Shared &);
};

template<class type> class TreeShared
{
public:
    TreeShared() { _ref = 0; m_parent = 0; /*counter++;*/ }
    TreeShared( type *parent ) { _ref=0; m_parent = parent; /*counter++;*/ }
    ~TreeShared() { /*counter--;*/ }

    void ref() { _ref++;  }
    void deref() { 
	if(_ref) _ref--; 
	if(!_ref && !m_parent) {
	    delete static_cast<type *>(this); 
	}
    }
    bool hasOneRef() { //kdDebug(300) << "ref=" << _ref << endl;
    	return _ref==1; }

    int refCount() const { return _ref; }
//    static int counter;

    void setParent(type *parent) { m_parent = parent; }
    type *parent() const { return m_parent; }
private:
    unsigned int _ref;
protected:
    type *m_parent;

private:
    TreeShared(const TreeShared &);
    TreeShared &operator=(const TreeShared &);
};

using KXMLCore::SharedPtr;
using KXMLCore::static_pointer_cast;
using KXMLCore::const_pointer_cast;

}

#endif

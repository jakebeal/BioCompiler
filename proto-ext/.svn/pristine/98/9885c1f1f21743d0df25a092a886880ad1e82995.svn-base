#ifndef  __XERCES_BODIES__
#define __XERCES_BODIES__

#include "odebody.h"
#include "odedynamics.h"
#include <map>
#include <vector>
//#include "DOMPrintErrorHandler.hpp"

#include <xercesc/dom/DOMErrorHandler.hpp>
#include <iostream>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLChar.hpp>

class ODEBody;
class ODEDynamics;


/**
 * Abstract class used for storing joint configurations
 * before they are created
 */
class XmlJoint {
public:
	/** id of first geom as defined in the XML doc **/
	std::string id1;
	/** id of second geom as defined in the XML doc **/
	std::string id2;

	/**
	 * A constructor which sets id1 and id2 based on element
	 * @param a DOM Element describing a joint
	 */
	XmlJoint(XERCES_CPP_NAMESPACE::DOMElement* element);

	/**
	 * Adds a joint between  bod1 and bod2 in the given world.
	 * @param world The ODE World where the joint is added
	 * @param bod1 First ODEBody the joint is attached too
	 * @param bod2 Other ODEBody the joint is attached too
	 */
	virtual dJointID createJoint(dWorldID world, dBodyID bod1, dBodyID bod2);
};

class XmlFixedJoint: public XmlJoint {
public:
	XmlFixedJoint(XERCES_CPP_NAMESPACE::DOMElement* element);
	dJointID createJoint(dWorldID world,dBodyID bod1, dBodyID bod2);
};

class XmlHingedJoint: public XmlJoint {
public:
	double *anchor;
	double *axis;
	dReal loStop;
	dReal hiStop;
	XmlHingedJoint(XERCES_CPP_NAMESPACE::DOMElement* element, const double* transform);
	dJointID createJoint(dWorldID world,dBodyID bod1, dBodyID bod2);
};

class XmlBody {
private:
public:

	std::string id;
	double* pos;
	double* quaternion;
	double mass;

	XmlBody(XERCES_CPP_NAMESPACE::DOMElement* element, const double* transform);
	virtual ODEBody* getODEBody(ODEDynamics* parent, Device* d);
};

class XmlBox: public XmlBody {
private:
	double* dim;
public:
	XmlBox(XERCES_CPP_NAMESPACE::DOMElement* element, const double* transform);
	ODEBody* getODEBody(ODEDynamics* parent, Device* d);

};


class XmlSphere: public XmlBody {
private:
	double radius;
public:
	XmlSphere(XERCES_CPP_NAMESPACE::DOMElement* element, const double* transform);
	ODEBody* getODEBody(ODEDynamics* parent, Device* d);

};

class XmlCylinder: public XmlBody {
private:
	double radius;
	double height;
public:
	XmlCylinder(XERCES_CPP_NAMESPACE::DOMElement* element, const double* transform);
	ODEBody* getODEBody(ODEDynamics* parent, Device* d);

};


class XmlCapsule: public XmlBody {
private:
	double radius;
	double height;
public:
	XmlCapsule(XERCES_CPP_NAMESPACE::DOMElement* element, const double* transform);
	ODEBody* getODEBody(ODEDynamics* parent, Device* d);

};

//TODO create destructor
class XmlWorldParser{
public:
	std::vector<XmlJoint*> jointList;
	std::vector<XmlBody*> bodyList;

	XmlWorldParser(){};
	bool process_xml(const char* xmlFile);

	void addJoint(xercesc::DOMElement* node, const double* pos);
	void addBody(xercesc::DOMElement* node, const double* pos);
	void processNode(xercesc::DOMTreeWalker* tw, const double* pos);
};



class BodiesInWorld : public xercesc::DOMNodeFilter{
public:
	BodiesInWorld() {};
	virtual xercesc::DOMNodeFilter::FilterAction acceptNode (const xercesc::DOMNode* node) const;
};
#endif

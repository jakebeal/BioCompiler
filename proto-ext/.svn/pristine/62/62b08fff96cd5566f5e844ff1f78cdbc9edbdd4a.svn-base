//g++ -g -Wall -pedantic -lxerces-c xerces_bodies.cpp -DMAIN_TEST -o parser
#include "xerces_bodies.h"

using namespace std;
using namespace xercesc;

#define POS_TAG "position"
#define ID_TAG "id"
#define QUAT_TAG "quaternion"
#define MASS_TAG "mass"

#define DIM_TAG "dim"

#define ANCHOR_TAG "anchor"
#define AXIS_TAG "axis"
#define HI_STOP_TAG "hiStop"
#define LOW_STOP_TAG "loStop"

/**
 *  Returns a child element with the given name
 *  @param element parent element
 *  @param name name of child element to retrieve
 *  @return the child element with the given name or NULL if there are 0 or >1 elements with the given name
 */
DOMNode* getElement(DOMElement* element, const char* name) {
	if (element == NULL)
		return NULL;

	DOMNodeList* elementList = element->getElementsByTagName(
			XMLString::transcode(name));
	if (elementList == NULL || elementList->getLength() != 1)
		return NULL;

	return elementList->item(0);
}

/**
 *  Returns the value of a names child element
 *  @param element parent element
 *  @param name name of child element value is stored in
 *  @return the value of the child element with the given name or NULL if there are 0 or >1 elements with the given name
 */
char* getChildValue(DOMElement* element, const char* name) {
	DOMNode* child = getElement(element, name);
	if (child == NULL)
		return NULL;

	DOMNodeList* contentList = child->getChildNodes();
	if (contentList->getLength() != 1)
		return NULL;

	DOMNode* firstContent = contentList->item(0);
	if (firstContent == NULL)
		return NULL;

	return XMLString::transcode(firstContent->getNodeValue());
}

/**
 *  Retrieve the value of an attribute
 *  @param node the DOMNode we are extracting an attribute from
 *  @param name the name of the attribute we want
 *  @return the value of the attribute or NULL if no such attribute is found
 */
char* getAttribute(DOMNode* node, const char* name) {
	if (node == NULL)
		return NULL;

	DOMNamedNodeMap* map = node->getAttributes();
	if (map == NULL)
		return NULL;

	DOMNode* attribute = map->getNamedItem(XMLString::transcode(name));
	if (attribute == NULL)
		return NULL;

	return XMLString::transcode(attribute->getNodeValue());
}

/**
 * Creates a rotation matrix from X-Y-Z fixed angles
 * @param angle contains X-Y-Z, angle[0] = x, angle[1] = y, angle[2] = z
 * @param matrix 3x3 matrix that become the rotation matrix
 */
void xyzFixedToRotMatrix(double* angle, double* matrix) {
	double sg = sin(angle[0]);
	double sb = sin(angle[1]);
	double sa = sin(angle[2]);

	double cg = cos(angle[0]);
	double cb = cos(angle[1]);
	double ca = cos(angle[2]);

	//first row
	matrix[0] = ca * cb;
	matrix[1] = ca * sb * sg + sa * cg;
	matrix[2] = ca * sb * cg + sa * sg;

	matrix[3] = sa * cb;
	matrix[4] = sa * sb * sg + ca * cg;
	matrix[5] = sa * sb * cg - ca * sg;

	matrix[6] = -sb;
	matrix[7] = cb * sg;
	matrix[8] = cb * cg;

}

/**
 * Creates a standard homogenous transformation matrix based upon a position
 * displacement and rotation
 * @param pos The position displacement
 * @param rotMatrix A 3x3 rotation matrix
 * @param transMatrix The 4x4 matrix that values are placed into
 */
void createMyTransformMatrix(double* pos, double* rotMatrix,
		double* transMatrix) {

	//row 1
	transMatrix[0] = rotMatrix[0];
	transMatrix[1] = rotMatrix[1];
	transMatrix[2] = rotMatrix[2];
	transMatrix[3] = pos[0];

	//row 2
	transMatrix[4] = rotMatrix[3];
	transMatrix[5] = rotMatrix[4];
	transMatrix[6] = rotMatrix[5];
	transMatrix[7] = pos[1];

	//row 3
	transMatrix[8] = rotMatrix[6];
	transMatrix[9] = rotMatrix[7];
	transMatrix[10] = rotMatrix[8];
	transMatrix[11] = pos[2];

	//row 4
	transMatrix[12] = 0;
	transMatrix[13] = 0;
	transMatrix[14] = 0;
	transMatrix[15] = 1;

}

/********************************************************
 ********************  Joints ***************************
 ********************************************************/

/***************************************************
 * XmlJoint implementation
 ***************************************************/
XmlJoint::XmlJoint(DOMElement* element) {

	DOMNode* id1element = getElement(element, "a");
	DOMNode* id2element = getElement(element, "b");
	if (id1element == NULL || id2element == NULL) {
		cout << "Joint improperly defined.";
		throw exception();
	}

	id1 = string(getChildValue(element, "a"));
	id2 = string(getChildValue(element, "b"));
}

//TODO this should be a pure virtual function
dJointID XmlJoint::createJoint(dWorldID world, dBodyID bod1, dBodyID bod2) {
	cout << "Unsupported argument" << endl;
	return NULL;
}

/***************************************************
 * XmlFixedJoint implementation
 ***************************************************/
XmlFixedJoint::XmlFixedJoint(DOMElement* element) :
	XmlJoint(element) {
}

dJointID XmlFixedJoint::createJoint(dWorldID world, dBodyID bod1, dBodyID bod2) {
	dJointID joint = dJointCreateFixed(world, 0);
	dJointAttach(joint, bod1, bod2);
	dJointSetFixed(joint);
	return joint;
}

/***************************************************
 * XmlHingedJoint implementation
 ***************************************************/
XmlHingedJoint::XmlHingedJoint(XERCES_CPP_NAMESPACE::DOMElement* element,
		const double* transform) :
	XmlJoint(element) {
	anchor = new double[3];
	axis = new double[3];

	/******************
	 * extract anchor *
	 ******************/
	DOMNode* anchorNode = getElement(element, ANCHOR_TAG);
	if (anchorNode == NULL) {
		cout << "Hinged joint anchor not specified correctly." << endl;
		throw exception();
	}

	char* anchorAtt[3];
	anchorAtt[0] = getAttribute(anchorNode, "x");
	anchorAtt[1] = getAttribute(anchorNode, "y");
	anchorAtt[2] = getAttribute(anchorNode, "z");

	for (int i = 0; i < 3; i++) {
		if (anchorAtt[i] == NULL) {
			cout << "Hinged joint anchor not specified correctly." << endl;
			throw exception();
		}
		anchor[i] = atof(anchorAtt[0]);
	}

	/********************
	 * extract position *
	 ********************/
	DOMNode* axisNode = getElement(element, AXIS_TAG);
	char* axisAtt[3];
	axisAtt[0] = getAttribute(axisNode, "x");
	axisAtt[1] = getAttribute(axisNode, "y");
	axisAtt[2] = getAttribute(axisNode, "z");
	for (int i = 0; i < 3; i++) {
		if (axisAtt[i] == NULL) {
			cout << "Hinged joint axis not specified correctly." << endl;
			throw exception();
		}
		axis[i] = atof(axisAtt[i]);
	}

	/*********************
	 * extract high stop *
	 *********************/
	char* hiStopChar = getChildValue(element, HI_STOP_TAG);
	//if high stop is not specified we assume there isn't one
	if (hiStopChar == NULL) {
		hiStop = -dInfinity;
	} else {
		hiStop = atof(hiStopChar);
	}

	//extract low stop
	char* loStopChar = getChildValue(element, LOW_STOP_TAG);
	//if low stop is not specified we assume there isn't one
	if (loStopChar == NULL) {
		loStop = dInfinity;
	} else {
		loStop = atof(loStopChar);
	}

	/**
	 * Apply Transform
	 */
	anchor[0] += transform[0];
	anchor[1] += transform[1];
	anchor[2] += transform[2];

}
dJointID XmlHingedJoint::createJoint(dWorldID world, dBodyID bod1, dBodyID bod2) {
	dJointID joint = dJointCreateHinge(world, 0);
	dJointAttach(joint, bod1, bod2);
	dJointSetHingeAnchor(joint, anchor[0], anchor[1], anchor[2]);
	dJointSetHingeAxis(joint, axis[0], axis[1], axis[2]);
	dJointSetHingeParam(joint, dParamLoStop, loStop);
	dJointSetHingeParam(joint, dParamHiStop, hiStop);
	return joint;
}

/********************************************************
 ********************  Bodies ***************************
 ********************************************************/

/***************************************************
 * XmlBody implementation
 ***************************************************/
XmlBody::XmlBody(DOMElement* element, const double* transform) {
	pos = new double[3];
	quaternion = new double[4];

	//Extract id
	char* idChar = getChildValue(element, ID_TAG);
	if (idChar == NULL)
		throw exception();
	id = string(idChar);

	//extract position
	DOMNode* posNode = getElement(element, POS_TAG);
	if (posNode == NULL)
		throw exception();

	char* posAtt[3];
	posAtt[0] = getAttribute(posNode, "x");
	posAtt[1] = getAttribute(posNode, "y");
	posAtt[2] = getAttribute(posNode, "z");
	for (int i = 0; i < 3; i++) {
		if (posAtt[i] == NULL)
			throw exception();
		pos[i] = atof(posAtt[i]);
	}

	//extract orientation
	DOMNode* quatNode = getElement(element, QUAT_TAG);
	if (quatNode != NULL) {
		char* quatAtt[4];
		quatAtt[0] = getAttribute(quatNode, "q1");
		quatAtt[1] = getAttribute(quatNode, "q2");
		quatAtt[2] = getAttribute(quatNode, "q3");
		quatAtt[3] = getAttribute(quatNode, "q4");
		for (int i = 0; i < 4; i++) {
			if (quatAtt[i] == NULL)
				throw exception();
			quaternion[i] = atof(quatAtt[i]);
		}
	}
	//extract mass
	char* massChar = getChildValue(element, MASS_TAG);
	if (massChar == NULL)
		throw exception();
	mass = atof(massChar);

	/**
	 * Apply transform
	 */
	pos[0] += transform[0];
	pos[1] += transform[1];
	pos[2] += transform[2];

}

ODEBody* XmlBody::getODEBody(ODEDynamics* parent, Device* d) {
	cout << "unimplemented method" << endl;
}

/***************************************************
 * XmlBox implementation
 ***************************************************/
XmlBox::XmlBox(DOMElement* element, const double* transform) :
	XmlBody(element, transform) {
	dim = new double[3];

	//extract position
	DOMNode* dimNode = getElement(element, DIM_TAG);
	if (dimNode == NULL)
		throw exception();

	char* dimAtt[3];
	dimAtt[0] = getAttribute(dimNode, "w");
	dimAtt[1] = getAttribute(dimNode, "b");
	dimAtt[2] = getAttribute(dimNode, "h");

	for (int i = 0; i < 3; i++) {
		if (dimAtt[i] == NULL)
			throw exception();
		dim[i] = atof(dimAtt[i]);
	}
}

ODEBody* XmlBox::getODEBody(ODEDynamics* parent, Device* d) {
	cout << pos[0] << endl;
	cout << pos[1] << endl;
	cout << pos[2] << endl;
	cout << dim[0] << endl;
	cout << dim[1] << endl;
	cout << dim[2] << endl;
	cout << mass << endl;

	return new ODEBox(parent, d, pos[0], pos[1], pos[2], dim[0], dim[1],
			dim[2], mass);
}

/***************************************************
 * XmlSphere implementation
 ***************************************************/
XmlSphere::XmlSphere(DOMElement* element, const double* transform) :
	XmlBody(element, transform) {
	//extract radius
	char* radiusChar = getChildValue(element, "radius");
	if (radiusChar == NULL)
		throw exception();

	radius = atof(radiusChar);
}

ODEBody* XmlSphere::getODEBody(ODEDynamics* parent, Device* d) {
	return new ODESphere(parent, d, pos, quaternion, radius, mass);
}

/***************************************************
 * XmlCylinder implementation
 ***************************************************/
XmlCylinder::XmlCylinder(DOMElement* element, const double* transform) :
	XmlBody(element, transform) {
	//extract radius
	char* radiusChar = getChildValue(element, "radius");
	if (radiusChar == NULL)
		throw exception();
	radius = atof(radiusChar);

	//extract height
	char* heightChar = getChildValue(element, "height");
	if (heightChar == NULL)
		throw exception();
	height = atof(heightChar);
}

ODEBody* XmlCylinder::getODEBody(ODEDynamics* parent, Device* d) {
	return new ODECylinder(parent, d, pos, quaternion, radius, height, mass);
}

/***************************************************
 * XmlCapsule implementation
 ***************************************************/
XmlCapsule::XmlCapsule(DOMElement* element, const double* transform) :
	XmlBody(element, transform) {
	//extract radius
	char* radiusChar = getChildValue(element, "radius");
	if (radiusChar == NULL)
		throw exception();
	radius = atof(radiusChar);

	//extract height
	char* heightChar = getChildValue(element, "height");
	if (heightChar == NULL)
		throw exception();
	height = atof(heightChar);
}

ODEBody* XmlCapsule::getODEBody(ODEDynamics* parent, Device* d) {
	return new ODECapsule(parent, d, pos, quaternion, radius, height, mass);
}

/********************************************************
 ********************  Parser ***************************
 ********************************************************/

/*****************************************************
 * XmlWorldParser implementation
 ***************************************************/
void XmlWorldParser::processNode(DOMTreeWalker* tw, const double* pos) {

	do {

		DOMElement* node = (DOMElement*) tw->getCurrentNode();

		//		printNode(node);

		/* Process body */
		if (XMLString::equals(XMLString::transcode("body"), node->getNodeName())) {
			cout << "adding a body" << endl;
			addBody(node, pos);
		} else
		/* Process joint */
		if (XMLString::equals(XMLString::transcode("joint"),
				node->getNodeName())) {
			addJoint(node, pos);
		} else
		/* Process a transform */
		if (XMLString::equals(XMLString::transcode("transform"),
				node->getNodeName())) {
			double trans[3];
			char* x_c = getAttribute(node, "x");
			char* y_c = getAttribute(node, "y");
			char* z_c = getAttribute(node, "z");

			if (x_c == NULL || y_c == NULL || z_c == NULL)
				throw exception();

			trans[0] = pos[0] + atof(x_c);
			trans[1] = pos[1] + atof(y_c);
			trans[2] = pos[2] + atof(z_c);

			if (tw->firstChild() != NULL) {
				processNode(tw, trans);
			}
		} else if (tw->firstChild() != NULL) {
			processNode(tw, pos);
		}

	} while (tw->nextSibling() != NULL);

	tw->parentNode();

	return;
}

void XmlWorldParser::addJoint(DOMElement* node, const double* pos) {
	string jointType = string(
			XMLString::transcode(
					node->getAttribute(XMLString::transcode("type"))));
	XmlJoint* joint;
	try {
		if (jointType == "fixed") {
			joint = new XmlFixedJoint((DOMElement*) node);
		} else if (jointType == "hinged") {
			joint = new XmlHingedJoint((DOMElement*) node, pos);
		} else {
			cout << "Unsupported joint type." << endl;
			throw "Unsupported joint type.";
		}
		jointList.push_back(joint);
	} catch (exception& e) {
		cout << "Poorly formed XML joint" << endl;
	}
}

void XmlWorldParser::addBody(DOMElement* node, const double* pos) {
	try {
		string bodyType = string(
				XMLString::transcode(
						node->getAttribute(XMLString::transcode("type"))));
		XmlBody* xBody;

		if (bodyType == "box") {
			xBody = new XmlBox(node, pos);
		} else if (bodyType == "sphere") {
			xBody = new XmlSphere(node, pos);
		} else if (bodyType == "cylinder") {
			xBody = new XmlCylinder(node, pos);
		} else if (bodyType == "capsule") {
			xBody = new XmlCapsule(node, pos);
		}	else {
			cout << "Unknown body type." << endl;
			throw "Unknown body type.";
		}
		bodyList.push_back(xBody);
	} catch (exception& e) {
		cout << "Poorly formed XML body " << endl;
	}
}

bool XmlWorldParser::process_xml(const char* xmlFile) {

	try {
		XMLPlatformUtils::Initialize();
	} catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		cout << "Error during initialization! :\n" << message << "\n";
		XMLString::release(&message);
		return 1;
	}

	XercesDOMParser* parser = new XercesDOMParser();
	parser->setValidationScheme(XercesDOMParser::Val_Always);
	parser->setDoNamespaces(true); // optional

	/**
	 * Configure parser error handling
	 */
	ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
	//	ErrorHandler *errHandler =(ErrorHandler*) new DOMPrintErrorHandler();
	parser->setErrorHandler(errHandler);

	try {
		parser->parse(xmlFile);
	} catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		cout << "Exception message is: \n" << message << "\n";
		XMLString::release(&message);
		return -1;
	} catch (const DOMException& toCatch) {
		char* message = XMLString::transcode(toCatch.msg);
		cout << "Exception message is: \n" << message << "\n";
		XMLString::release(&message);
		return -1;
	} catch (const SAXException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		cout << "Exception message is: " << message << "\n";
		XMLString::release(&message);
		return -1;
	} catch (const exception& toCatch) {
		cout << "Unexpected Exception \n" << toCatch.what() << endl;
		return -1;
	}

	/**
	 * Walk through the document, adding bodies and joints in their relative frames
	 */
	DOMDocument* doc = parser->getDocument();
	DOMTreeWalker* walker = doc->createTreeWalker(doc->getDocumentElement(),
			DOMNodeFilter::SHOW_ELEMENT, new BodiesInWorld(), true);
	/** Initial world frame */
	double transform[3] = { 0, 0, 0 };
	processNode(walker, transform);

	//TODO Ensure that I am cleaning up everything I need to
	/** Clean up no longer needed resources **/
	doc->release();
	delete errHandler;

	return true;
}

/***************************************************
 * BodiesInWorld Implementation
 ***************************************************/
DOMNodeFilter::FilterAction BodiesInWorld::acceptNode(const DOMNode* node) const {
	if (node->getNodeType() == 1) {

		string name = string(XMLString::transcode(node->getNodeName()));
		if (name.compare("body") == 0)
			return FILTER_ACCEPT;

		if (name.compare("joint") == 0)
			return FILTER_ACCEPT;

		if (name.compare("transform") == 0)
			return FILTER_ACCEPT;

		if (name.compare("root") == 0)
			return FILTER_SKIP;
		if (name.compare("bodies") == 0)
			return FILTER_SKIP;
		if (name.compare("joints") == 0)
			return FILTER_SKIP;

	}

	return FILTER_REJECT;
}

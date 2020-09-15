#include "odebodyfactory.h"
//#include "xerces_bodies.h"

ODEBodyFactory::ODEBodyFactory(const char* body_file,ODEDynamics* parent) {
  this->parent = parent;
	bodySize = 1.0;
	defaultSize = 10.0;
	x = 0; // -5
	y = 0.0;
	z = 5.5;
	space = 10.0;
	count = 0;

	cout << "Creating parser..." << endl;
	parser = new XmlWorldParser();
	parser->process_xml(body_file);
	cout << "Parser created." << endl;
}

const char* ODEBodyFactory::getBodyFile() {
	return body_file;
}

void ODEBodyFactory::setBodyFile(const char* file) {
	body_file = file;
}

void ODEBodyFactory::create_joints() {
	cout << "Creating joints" << endl;
	while (!parser->jointList.empty()) {
		XmlJoint* joint = parser->jointList.back();

		map<string, ODEBody*>::iterator iter = bodyMap.begin();

		dBodyID bod1;
		dBodyID bod2;
		if ( joint->id1.compare("world") == 0) {
			bod1 = 0;
		} else {
			iter = bodyMap.find(joint->id1);
			if (iter == bodyMap.end()) {
				throw "missing body";
			} else {
				bod1 = iter->second->body;
			}
		}

		if ( joint->id2.compare("world") == 0) {
			bod2 = 0;
		} else {
			iter = bodyMap.find(joint->id2);
			if (iter == bodyMap.end()) {
				throw "missing body";
			} else {
				bod2 = iter->second->body;
			}
		}

		dJointID jointId = joint->createJoint(parent->world, bod1, bod2);

                /*		if( joint->id1.find("DriveWheel") != -1 || joint->id2.find("DriveWheel") != -1){
			dJointSetHingeParam(jointId, dParamFMax, 50);
			dJointSetHingeParam(jointId, dParamVel, -0.2);
			cout<<"Drive wheel found!"<<endl;
		}else{
			if( joint->id1.find("Wheel", 5) != -1 || joint->id2.find("Wheel", 5) != -1)
				cout<<"Not driving on "<< joint->id1 <<" or " <<joint->id2<<endl;
                                }*/

		parser->jointList.pop_back();
	}


	cout<<"Created joints"<<endl;


}

bool ODEBodyFactory::empty() {
	return parser->bodyList.empty();
}

int ODEBodyFactory::numBodies() {
	return parser->bodyList.size();
}

ODEBody* ODEBodyFactory::next_body(Device* d) {
	XmlBody* nextBody = parser->bodyList.back();
	if (nextBody == NULL) {
		cout << "BODY IS NULL" << endl;
	}
	ODEBody* b = ((XmlBox*) nextBody)->getODEBody(parent, d);
	pair<map<string, ODEBody*>::iterator, bool> ret = bodyMap.insert(pair<
			string, ODEBody*> (nextBody->id, b));
	parser->bodyList.pop_back();
	delete nextBody;


	if (parser->bodyList.empty()) {
		this->create_joints();
	}

	if (ret.second) {
		return b;
	} else {
		throw "duplicate body id";
	}

}

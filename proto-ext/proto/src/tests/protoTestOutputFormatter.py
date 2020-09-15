'''
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and
contributors listed in the AUTHORS file in the MIT Proto
distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.
'''

'''
This script is designed to read the Proto *.RESULTS files and convert
them to Ant JUnit XML format. This will enable build tool Hudson to
provide a visual display of the test results.
'''

import sys, glob, os
import xml.etree.cElementTree as ET


class TestCase:
    def __init__(self,className,name):
        self.className = className
        self.name = name
        self.time = "0.0"
        self.failMessage = ""
	self.errorMessage = ""
        self.failContent = ""
        self.dumpFilePath = ""
        self.argStr = ""
        self.randomSeed = ""
        
    def __str__(self):
        output = "TestCase: " + self.className + " " + self.name + \
              " \n" + self.argStr + " " + self.dumpFilePath + " " + \
              self.randomSeed + " \n" + self.failMessage + " \n" + \
              self.failContent + "\n\n"
              
        return output
        
# This class encapsulates the attributes of the testsuite tag.
# There is one such object per RESULTS file.
class TestSuite:
    def __init__(self,name):
        self.name = name
        self.errors = 0
        self.failures = 0
        self.tests = 0
        self.time = "0.0"
        self.timestamp = ""
        self.protoVersion = ""
        self.hostname = ""
        
    def __str__(self):
        output = "TestSuite: " + self.name + " Failures: " + \
              str(self.failures) + " Errors: " + str(self.errors) + \
	      " " + str(self.tests) + " \n" + self.protoVersion + "\n"
              
        return output
# Each one of these objects will correspond to a singe RESULTS file.
class ResultFileObject:

    def __init__(self):
        self.propertyDict = {}
        self.testCaseList = []
        self.protoVersion = ""
        self.testSuite = None
        self.elementTree = None
        
    def createXML(self):
        root = ET.Element("testsuite")
        root.set("errors", str(self.testSuite.errors))
        root.set("failures",str(self.testSuite.failures))
        root.set("hostname", "")
        root.set("name", self.testSuite.name)
        root.set("tests", str(self.testSuite.tests))
        root.set("time", "0.0")
        root.set("timestamp", "")
        
        properties = ET.SubElement(root, "properties")
        
        property = ET.SubElement(properties,"property")
        property.set("ProtoVersion", self.protoVersion)
        
        for item in self.testCaseList:
            property1 = ET.SubElement(properties,"property")
            property2 = ET.SubElement(properties,"property")
            property3 = ET.SubElement(properties,"property")
            
            property1.set("name","".join(item.name.split()) + "Args")
            property1.set("value", item.argStr)
            property2.set("name", "".join(item.name.split()) + "DumpFilePath")
            property2.set("value", item.dumpFilePath)
            
            if len(item.randomSeed) > 0:
                property3.set("name", "".join(item.name.split()) + "RandomSeed")
                property3.set("value",item.randomSeed)
            
        for item in self.testCaseList:
            testcase = ET.SubElement(root,"testcase")
            testcase.set("classname",item.className)
            testcase.set("name", item.name)
            testcase.set("time", "0.0")
            
            if len(item.failMessage) > 0:
                failure = ET.SubElement(testcase,"failure")
                failure.set("message", item.failMessage)
                failure.set("type", "failure")
                failure.text = item.failContent
	    if len(item.errorMessage) > 0:
		error = ET.SubElement(testcase, "error")
		error.set("message", "PROTO test error")
		error.set("type", "error")
		error.text = item.errorMessage
                
        
        systemout = ET.SubElement(root,"system-out")
        systemout.text = "<![CDATA[]]>"
        systemerr = ET.SubElement(root,"system-err")
        systemerr.text = "<![CDATA[]]>" 
        
        self.elementTree = ET.ElementTree(root)

# output XML files are written to /proto/src/tests/xml
# with file names xxxx.test.xml
    def writeToFile(self):
        if ((sys.platform.startswith("linux")) or (sys.platform.startswith("darwin"))):
            directory = "xml/"  # this is for linux and OS X
        else:
            directory = "xml/"  # this is for Windows
        try:
            os.makedirs(directory)
        except OSError:
            if os.path.isdir(directory):
                # We are nearly safe
                pass
            else:
                # There was an error on creation, so make sure we know about it
                raise
        fileName = directory + self.testSuite.name + ".xml"
        #self.elementTree.write(sys.stdout,"UTF-8")
        self.elementTree.write(fileName,"UTF-8")
        
  
class ProcessFiles:

    def __init__(self):
        self.resultFileObjectList = []
        self.fileNameList = []
        
    def readResultFiles(self,*args):
        #print args

        for file in args:
            self.fileNameList.append(file)

        
    def processFiles(self, *args):

        self.readResultFiles(args)

        for i in range(len(self.fileNameList[0][0])):
            #print self.fileNameList[0][0][i]
            self.putDataInTestCases(self.fileNameList[0][0][i])

        for result in self.resultFileObjectList:
            result.createXML()
            result.writeToFile()
            
        
    def putDataInTestCases(self,file):

        testName = '.'.join(file.split(".")[0:2])  
        resultFileObject = ResultFileObject()
        
        try:
            input = open(file,"r")
        
	    inputLines = input.readlines()
	    
            testSuite = self.parseSuiteData(inputLines,testName)
            
            resultFileObject.testSuite = testSuite

	    testCaseLines = []
	    count = 0
	    line = ""

	    while count < len(inputLines):
	    
	        while not(line.startswith("---")):
		    line = inputLines[count]
		    testCaseLines.append(line)
		    count = count + 1

	        testCase = self.parseTestCaseLines(testName,testCaseLines)
	        resultFileObject.testCaseList.append(testCase)

	        testCaseLines = []
	        if count < len(inputLines):
		    line = inputLines[count]

        except IOError, io:
            print "Error opening test result file: %s" (file) + str(io)
        finally:
	    input.close()
	    
	self.resultFileObjectList.append(resultFileObject)
     
     
    def parseTestCaseLines(self,testSuiteName,lines):
        #print lines
        
        if not lines[0].startswith("Test"):
            del lines[0]
            
        if lines[0].startswith("Test"):
            testName = lines[0].rstrip()
            
        testCase = TestCase(testSuiteName, testName)
        
        if lines[1].startswith("Running"):
            testCase.argStr = lines[1][lines[1].index(":") + 1: ].strip()

        if lines[2].startswith("Dump"):
            testCase.dumpFilePath = lines[2][lines[2].index(":") + 1: ].strip()
            
        randSeedPos = self.getLineIndexes(lines,"Using")
        if len(randSeedPos) > 0:
            testCase.randomSeed = lines[randSeedPos[0]].split()[3]
            
        failLinePos = self.getLineIndexes(lines, "FAIL")
        
        if len(failLinePos) > 0:
            failMessage = lines[failLinePos[0]]
            failMessage = failMessage[failMessage.index(":") + 1:].strip()
            testCase.failMessage = failMessage
            for i in range(1,len(failLinePos)):
               testCase.failContent += lines[failLinePos[i]]

	errorLinePos = self.getLineIndexes(lines, "Proto terminated with a non-zero")
	if len(errorLinePos) > 0:
            protoOutput = self.getLineIndexes(lines, "***PROTO OUTPUT***")
	    for i in range(protoOutput[0], errorLinePos[0]):
	       testCase.errorMessage += lines[i]
	  
        return testCase
     
     
    def parseSuiteData(self,inputLines, testName):
        testSuite = TestSuite(testName)
        
        firstLine = inputLines[0]
        
        protoLine = self.getProtoVersion(inputLines)
        testSuite.protoVersion = protoLine
        
        if firstLine.startswith("All"):
            testSuite.failures = 0
	    testSuite.errors = 0
            testSuite.tests = self.getNumLinesStartWith(inputLines,"Test")
        elif firstLine.startswith("Passed"):
            testSuite.tests = int(firstLine.split()[4])
	    passed = int(firstLine.split()[1])
            testSuite.failures = self.getNumLinesStartWith(inputLines,"FAIL: Failed")
	    testSuite.errors = testSuite.tests - testSuite.failures - passed
    
        return testSuite
        
        
    def getNumLinesStartWith(self,inputLines,str):
        count = 0
        for line in inputLines:
            if line.startswith(str):
                count += 1
        return count
        
    def getLineIndexes(self,lines, startStr):
        indexList = []
        for i in range(len(lines)):
            if lines[i].startswith(startStr):
                indexList.append(i)
                
        return indexList
    
    def getProtoVersion(self,inputLines):
        count = 0
        for line in inputLines:
            if line.startswith("PROTO"):
                break
            count += 1
            
        protoLine = inputLines[count].rstrip()
        return protoLine
        
        
    def outputTestCases(self):
        
        for item in self.resultFileObjectList:
            print item.testSuite
            for x in item.testCaseList:
                print x
      
        
       
def main():
  pf = ProcessFiles()
  #print sys.argv
  sys.argv = [item for arg in sys.argv for item in glob.glob(arg)]
  pf.processFiles(sys.argv[1:])
   
       
if __name__ == "__main__":
    main()   

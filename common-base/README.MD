
Common Modeling Framework

### Dependencies
The common-base dependencies are in the lib folder

### How to Build just common-base.jar
1. Download Eclipse Luna
2. Import CMF as an Eclipse project
3. Update .classpath references to the external required jars stored in the lib folder
4. Build with Ant
  1. Right click on build_just_cmf.xml and select Run As and then Ant Build.  This should fail, but will setup a default Ant Build configuration
  2. Right click on build_just_cmf.xml and select Run As and then External tools configurations
  2. Go to the Targets tab and check "release"
  3. Click Run to build 
5. common-base.jar in the release folder is the release version

/*
        This file is part of the MR simulation project
        Date: 03/2006
        Authors:  T. Stoecker, J. Dai
        MR group, Institute of Medicine, Research Centre Juelich, Germany
*/

#include "XmlSequence.h"

/*****************************************************************************/
 ConcatSequence* XmlSequence::getSequence(bool verbose ){
	ConcatSequence* pSeq = (ConcatSequence*)Transform( getRoot() );
	pSeq->Prepare(false);
	bool bstatus = pSeq->Prepare(true);
	if (verbose)
	{
		pSeq->writeSeqDiagram("seq.bin");
		cout << endl << "Sequence  '"<< pSeq->getName() << "' created " ;
		if (bstatus)	cout << " sucessfully ! " << endl;
		else		cout << " with errors/warings (see above) " << endl;
		cout << " - total sequence duration = " << pSeq->getDuration() << " msec " << endl;
		cout << " - sequence diagram stored to seq.bin " << endl;
		cout << endl << "Dump of sequence tree: " << endl << endl;
		pSeq->getInfo(0);
	}
	return pSeq;
 };

/*****************************************************************************/
 Sequence* XmlSequence::Transform(DOMNode* node){
	string name;
	Sequence *pSeq=NULL, *pChildLast=NULL, *pChildNext=NULL;
	if (!node) return NULL;

	name = XMLString::transcode(node->getNodeName()) ;
	if (name == "ConcatSequence" )
	{
			CreateConcatSequence(&pSeq,node);
			DOMNode* child ;
			for (child = node->getFirstChild(); child != 0; child=child->getNextSibling())
			{
				pChildNext=Transform(child);
				if (pChildNext != NULL)
				{
					//cout << pSeq->getName() << ": inserting " << pChildNext->getName() ;   
					//if (pChildLast!=NULL) cout << " after " << pChildLast->getName() ;   
					((ConcatSequence*)pSeq)->InsertChild( pChildLast, pChildNext );
 					pChildLast=pChildNext;
					//cout << " successful!" << endl;
				}
			}
			return pSeq;
	}

	//add new seq types to this list and implement creator function below
	if(name == "AtomicSequence" )	{ CreateAtomicSequence(&pSeq,node);	return pSeq; }
	if(name == "DelayAtom"	)	{ CreateDelayAtom(&pSeq,node);		return pSeq; }
	if(name == "GradientSpiralExtRfConcatSequence"	){ CreateGradientSpiralExtRfConcatSequence(&pSeq,node);	return pSeq; }

	return NULL; //if the node name is not known
 };
 
 /*************************************************************************************************
  implementation of conversion for different sequence types: new types need to be added accordingly
  *************************************************************************************************/

/*****************************************************************************/
 void XmlSequence::CreateConcatSequence(Sequence** pSeq, DOMNode* node){
	string name="ConcatSequence",item,value;
	int reps=1,iTreeSteps=0;
	double factor=1.0;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")		name = value;
			if (item=="Repetitions")	reps = atoi(value.c_str() );
			if (item=="ConnectToLoop")	iTreeSteps = atoi(value.c_str());
			if (item=="Factor")		factor = atof(value.c_str() );
		}
	}
	*pSeq = new ConcatSequence(name,reps);
	if (iTreeSteps>0) (*pSeq)->setTreeSteps(iTreeSteps);
	(*pSeq)->setFactor(factor);
	if (item=="Repetitions" && value=="Nx")	{(*pSeq)->NewParam(true); ((ConcatSequence*)*pSeq)->getLoopMethod(1); }
	if (item=="Repetitions" && value=="Ny")	{(*pSeq)->NewParam(true); ((ConcatSequence*)*pSeq)->getLoopMethod(2); }

	//get parameters for this ConcatSequence
	DOMNode* child ;
	for (child = node->getFirstChild(); child != 0; child=child->getNextSibling())
	{
		name = XMLString::transcode(child->getNodeName()) ;
		pAttributes = child->getAttributes();
		if (name=="Parameter" && pAttributes)
		{
			int nSize = pAttributes->getLength();
			for(int i=0;i<nSize;++i)
			{
				DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
				item = XMLString::transcode(pAttributeNode->getName());
				value = XMLString::transcode(pAttributeNode->getValue());
				if (item=="TR")		(*pSeq)->getParameter()->setTR( atof(value.c_str() ) );
				if (item=="TE")		(*pSeq)->getParameter()->setTE( atof(value.c_str() ) );
				if (item=="TI")		(*pSeq)->getParameter()->setTI( atof(value.c_str() ) );
				if (item=="TD")		(*pSeq)->getParameter()->setTD( atof(value.c_str() ) );
				if (item=="Nx")		(*pSeq)->getParameter()->setNx( atoi(value.c_str() ) );
				if (item=="Ny")		(*pSeq)->getParameter()->setNy( atoi(value.c_str() ) );
				if (item=="FOVx")	(*pSeq)->getParameter()->setFOVx( atof(value.c_str() ) );
				if (item=="FOVy")	(*pSeq)->getParameter()->setFOVy( atof(value.c_str() ) );
				if (item=="ReadBW")	(*pSeq)->getParameter()->setReadBW( atof(value.c_str() ) );
			}
		}
	}
 };

/*****************************************************************************/
 void XmlSequence::CreateAtomicSequence(Sequence** pSeq, DOMNode* node){
	string name,item,value;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")		name = value;
		}
	}
	*pSeq = new AtomicSequence(name);
	//set pulses
	DOMNode* child ;
	for (child = node->getFirstChild(); child != 0; child=child->getNextSibling())
	{
		PulseShape* pPulse = NULL;
		int iTreeSteps=0;
		CreatePulseShape(&pPulse, &iTreeSteps, child);
		if (pPulse!=NULL)
			((AtomicSequence*)*pSeq)->setPulse(pPulse,iTreeSteps);
	}
 };

/*****************************************************************************/
 void XmlSequence::CreateDelayAtom(Sequence** pSeq, DOMNode* node){
	string S1="NULL",S2="NULL",name="DelayAtom",item,value;
	double delay,factor=-1.0;
	int iNADC=0;
	DelayType DT;
	bool bUseTE=false, bUseHalfTE=false, bUseTR=false, bUseTI=false, bUseTD=false;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="Delay")	delay = atof(value.c_str()); //set numeric delay
			if (item=="Factor")	factor = atof(value.c_str()); //set numeric delay
			if (item=="Delay" && value=="TE")	bUseTE=true ;		//use TE delay from parameter
			if (item=="Delay" && value=="TE/2")	bUseHalfTE=true ;	//use TE delay from parameter
			if (item=="Delay" && value=="TR")	bUseTR=true ;		//use TR delay from parameter
			if (item=="Delay" && value=="TI")	bUseTI=true ;		//use TI delay from parameter
			if (item=="Delay" && value=="TD")	bUseTD=true ;		//use TD delay from parameter
			if (item=="StartSeq")	S1= value;
			if (item=="StopSeq")	S2= value;
			if (item=="ADCs")	iNADC = atoi(value.c_str());
			if (item=="DelayType")	
			{
                        	if ( value== "B2E" ) DT = DELAY_B2E ;
                        	if ( value== "C2E" ) DT = DELAY_C2E ;
                        	if ( value== "B2C" ) DT = DELAY_B2C ;
                        	if ( value== "C2C" ) DT = DELAY_C2C ;
			}
		}
	}
	*pSeq = new DelayAtom(delay,NULL,NULL,DT,iNADC,name);
	((DelayAtom*)*pSeq)->setStartStopSeq(S1,S2);
	if (bUseHalfTE) ((DelayAtom*)*pSeq)->useHalfTE(); 
	if (bUseTE) ((DelayAtom*)*pSeq)->useTE();
	if (bUseTR) ((DelayAtom*)*pSeq)->useTR();
	if (bUseTI) ((DelayAtom*)*pSeq)->useTI();
	if (bUseTD) ((DelayAtom*)*pSeq)->useTD();
	if (factor>0.0) ((DelayAtom*)*pSeq)->setFactor(factor);
 };

/*****************************************************************************/
 void XmlSequence::CreateGradientSpiralExtRfConcatSequence(Sequence** pSeq, DOMNode* node){
	string filename,name="GradientSpiralExtRfConcatSequence",item,value;
	double dturns=32.0, dtune=0.2, dres=1.0;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="RfFileName")	filename = value;
			if (item=="Turns")	dturns=atof(value.c_str());
			if (item=="Tune")	dtune=atof(value.c_str());
			if (item=="Resolution")	dres=atof(value.c_str());
		}
	}
	*pSeq = new GradientSpiralExtRfConcatSequence(filename,dturns,dtune,dres,name);
 };
 /***************************************************************************************************
  implementation of conversion for different pulseshape types: new types need to be added accordingly
  ***************************************************************************************************/

/*****************************************************************************/
 void XmlSequence::CreatePulseShape(PulseShape** pPulse, int* iTreeSteps, DOMNode* node){
	string name,item,value;
	int iNADC=0;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="ConnectToLoop")	*iTreeSteps = atoi(value.c_str());
			if (item=="ADCs")		iNADC = atoi(value.c_str());
		}
	}
	name = XMLString::transcode(node->getNodeName()) ;
	//add new pulse shapes to this list and implement creator function below
	if ( name == "EmptyPulse"	  ) CreateEmptyPulse(pPulse, node);
	if ( name == "ExternalPulseShape" ) CreateExternalPulseShape(pPulse, node);
	if ( name == "HardRfPulseShape"   ) CreateHardRfPulseShape(pPulse, node);
	if ( name == "RfReceiverPhase"	  ) CreateRfReceiverPhase(pPulse, node);
	if ( name == "RfPhaseCycling"	  ) CreateRfPhaseCycling(pPulse, node);
	if ( name == "RfSpoiling"	  ) CreateRfSpoiling(pPulse, node);
	if ( name == "TGPS"		  ) CreateTGPS(pPulse, node);
	if ( name == "PE_TGPS"		  ) CreatePE_TGPS(pPulse, node);
	if ( name == "RO_TGPS"		  ) CreateRO_TGPS(pPulse, node);
	if ( name == "GradientSpiral"	  ) CreateGradientSpiral(pPulse, node);
	//add ADCs to pulse shape, if not already done
	if (*pPulse != NULL)
		if (iNADC>0 && (*pPulse)->getNumOfADCs() == 0)
			(*pPulse)->setNumOfADCs(iNADC);
 };

/*****************************************************************************/
 void XmlSequence::CreateEmptyPulse(PulseShape** pPulse, DOMNode* node){
	string name="EmptyPulse",item,value;
	double duration=0.0;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="Duration")	duration = atof(value.c_str());
		}
	}
	*pPulse = new EmptyPulse(duration, 0, name);
 };

/*****************************************************************************/
 void XmlSequence::CreateExternalPulseShape(PulseShape** pPulse, DOMNode* node){
	string name="ExternalPulseShape",filename,item,value;
	double factor=1.0;
	PulseAxis eAxis=AXIS_GX;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="FileName")	filename = value;
			if (item=="Factor")	factor = atof(value.c_str());
			if (item=="Axis")
				{
                        		if ( value == "RF" ) eAxis = AXIS_RF ;
                        		if ( value == "GX" ) eAxis = AXIS_GX ;
                        		if ( value == "GY" ) eAxis = AXIS_GY ;
                        		if ( value == "GZ" ) eAxis = AXIS_GZ ;
				}
		}
	}
	*pPulse = new ExternalPulseShape(filename, eAxis, factor, name);
 };

/*****************************************************************************/
 void XmlSequence::CreateHardRfPulseShape(PulseShape** pPulse, DOMNode* node){
	string name="HardRfPulseShape",item,value;
	double duration=0.0,phase=0.0,flipangle=0.0;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="Duration")	duration = atof(value.c_str());
			if (item=="FlipAngle")	flipangle = atof(value.c_str()); 
			if (item=="Phase")	phase = atof(value.c_str());
		}
	}
	if (flipangle == 0.0 ) cout << name << " warning: zero flipangle" << endl;
	if (duration == 0.0 ) cout << name << " warning: zero duration" << endl;
	*pPulse = new HardRfPulseShape(flipangle, phase, duration, name);
 };

/*****************************************************************************/
 void XmlSequence::CreateRfReceiverPhase(PulseShape** pPulse, DOMNode* node){
	string name="RfReceiverPhase",item,value;
	double phase=0.0;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="Phase")	phase = atof(value.c_str());
		}
	}
	*pPulse = new RfReceiverPhase(phase, name);
 };

/*****************************************************************************/
 void XmlSequence::CreateRfPhaseCycling(PulseShape** pPulse, DOMNode* node){
	string name="RfPhaseCycling",item,value;
	double phases[128],duration=0.0;
	int cycle=1;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="Duration")	duration = atof(value.c_str());
			if (item=="Cycle")	cycle = atoi(value.c_str());
			if (item=="Phase1")	phases[0] = atof(value.c_str());
			if (item=="Phase2")	phases[1] = atof(value.c_str());
			if (item=="Phase3")	phases[2] = atof(value.c_str());
			if (item=="Phase4")	phases[3] = atof(value.c_str());
		}
	}
	*pPulse = new RfPhaseCycling(&phases[0], cycle, duration, name);
 };

/*****************************************************************************/
 void XmlSequence::CreateRfSpoiling(PulseShape** pPulse, DOMNode* node){
	string name="RfSpoiling",item,value;
	double phase=0.0,duration=0.0;
	int startcycle=0;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")		name = value;
			if (item=="QuadPhaseInc")	phase = atof(value.c_str());
			if (item=="Duration")		duration = atof(value.c_str());
			if (item=="StartCycle")		startcycle = atoi(value.c_str());
		}
	}
	*pPulse = new RfSpoiling(phase, duration, startcycle, name);
 };

/*****************************************************************************/
 void XmlSequence::CreateTGPS(PulseShape** pPulse, DOMNode* node){
	string name="TGPS",item,value,pulse_name;
	double area=0.0,factor=1.0,duration=0.0, slewrate=-1.0, maxampl=-1.0,asymsr=0.0;
	PulseAxis eAxis=AXIS_GX;
	int getareamethod=0;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="Area")	pulse_name = value;
			if (item=="Area")	area = atof(value.c_str());
			if (item=="Gmax")	maxampl = atof(value.c_str());
			if (item=="SlewRate")	slewrate = atof(value.c_str());
			if (item=="AsymSR")	asymsr = atof(value.c_str());
			if (item=="Factor")	factor = atof(value.c_str());
			if (item=="Duration")	duration = atof(value.c_str());
			if (item=="Area" && value=="KMAXx")	getareamethod=1 ;
			if (item=="Area" && value=="KMAXy")	getareamethod=2 ;
			if (item=="Area" && value=="KMAXz")	getareamethod=3 ;
			if (item=="Area" && value=="DKx")	getareamethod=4 ;
			if (item=="Area" && value=="DKy")	getareamethod=5 ;
			if (item=="Area" && value=="DKz")	getareamethod=6 ;
			if (item=="Axis")
				{
                        		if ( value == "GX" ) eAxis = AXIS_GX ;
                        		if ( value == "GY" ) eAxis = AXIS_GY ;
                        		if ( value == "GZ" ) eAxis = AXIS_GZ ;
				}
		}
	}
	*pPulse = new TGPS(area, eAxis, name);
	((GradientPulseShape*)*pPulse)->setFactor(factor);
	if (maxampl>0.0) ((GradientPulseShape*)*pPulse)->setMaxAmpl(maxampl);
	if (slewrate>0.0) ((GradientPulseShape*)*pPulse)->setSlewRate(slewrate);
	if (asymsr!=0.0) ((TGPS*)*pPulse)->setAsymSR(asymsr);
	if (duration > 0.0) ((GradientPulseShape*)*pPulse)->NewDuration(duration);

	((GradientPulseShape*)*pPulse)->LinkToPulse(pulse_name); //get area from another pulse

	if (getareamethod>0) //get area from seq-root parameters (KMAXx or KMAXy)
		((GradientPulseShape*)*pPulse)->getAreaMethod(getareamethod);
 };

/*****************************************************************************/
 void XmlSequence::CreateRO_TGPS(PulseShape** pPulse, DOMNode* node){
	string name="RO_TGPS",item,value;
	double area=1.0,flat_top_duration=1.0,factor=1.0, slewrate=-1.0, maxampl=-1.0,asymsr=0.0;
	int iNADC=0;
	PulseAxis eAxis=AXIS_GX;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="Area")	area = atof(value.c_str());
			if (item=="Factor")	factor = atof(value.c_str());
			if (item=="Gmax")	maxampl = atof(value.c_str());
			if (item=="SlewRate")	slewrate = atof(value.c_str());
			if (item=="AsymSR")	asymsr = atof(value.c_str());
			if (item=="FlatTop")	flat_top_duration = atof(value.c_str());
			if (item=="ADCs")	iNADC = atoi(value.c_str());
			if (item=="Axis")
				{
                        		if ( value == "GX" ) eAxis = AXIS_GX ;
                        		if ( value == "GY" ) eAxis = AXIS_GY ;
                        		if ( value == "GZ" ) eAxis = AXIS_GZ ;
				}
		}
	}
	*pPulse = new RO_TGPS(area, flat_top_duration, iNADC, eAxis, name);
	((GradientPulseShape*)*pPulse)->setFactor(factor);
	if (maxampl>0.0) ((GradientPulseShape*)*pPulse)->setMaxAmpl(maxampl);
	if (slewrate>0.0) ((GradientPulseShape*)*pPulse)->setSlewRate(slewrate);
	if (asymsr!=0.0) ((TGPS*)*pPulse)->setAsymSR(asymsr);
	((TGPS*)*pPulse)->Prepare(false);
	if (iNADC==0)
	{
		((GradientPulseShape*)*pPulse)->setFactor(factor);
		if (eAxis==AXIS_GX) ((GradientPulseShape*)*pPulse)->getAreaMethod(1);
		if (eAxis==AXIS_GY) ((GradientPulseShape*)*pPulse)->getAreaMethod(2);
		if (eAxis==AXIS_GZ) ((GradientPulseShape*)*pPulse)->getAreaMethod(3);
	}
 };

/*****************************************************************************/
 void XmlSequence::CreatePE_TGPS(PulseShape** pPulse, DOMNode* node){
	string name="PE_TGPS",item,value;
	double area=0.0,factor=1.0, duration=-1.0, slewrate=-1.0, maxampl=-1.0,asymsr=0.0;
	int steps=0; bool noramps=false;
	PulseAxis eAxis=AXIS_GY;
	PE_ORDER order=LINEAR_UP;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="Factor")	factor = atof(value.c_str());
			if (item=="Area")	area = atof(value.c_str());
			if (item=="Gmax")	maxampl = atof(value.c_str());
			if (item=="SlewRate")	slewrate = atof(value.c_str());
			if (item=="AsymSR")	asymsr = atof(value.c_str());
			if (item=="NoRamps" && value=="true")	noramps=true;
			if (item=="Duration")	duration = atof(value.c_str());
			if (item=="Steps")	steps = atoi(value.c_str());
			if (item=="Axis")
				{
                        		if ( value == "GX" ) eAxis = AXIS_GX ;
                        		if ( value == "GY" ) eAxis = AXIS_GY ;
                        		if ( value == "GZ" ) eAxis = AXIS_GZ ;
				}
			if (item=="Order")
				{
                        		if ( value == "LINEAR_UP"   ) order = LINEAR_UP   ;
                        		if ( value == "LINEAR_DN"   ) order = LINEAR_DN   ;
                        		if ( value == "CENTRIC_OUT" ) order = CENTRIC_OUT ;
                        		if ( value == "CENTRIC_IN"  ) order = CENTRIC_IN  ;
				}
		}
	}
	*pPulse = new PE_TGPS(area, steps, order, eAxis, name);
	if (maxampl>0.0) ((GradientPulseShape*)*pPulse)->setMaxAmpl(maxampl);
	if (slewrate>0.0) ((GradientPulseShape*)*pPulse)->setSlewRate(slewrate);
	if (asymsr!=0.0) ((TGPS*)*pPulse)->setAsymSR(asymsr);
	if (noramps) ((TGPS*)*pPulse)->NoRamps();
	if (duration > 0.0) ((PE_TGPS*)*pPulse)->NewDuration(duration);
	((GradientPulseShape*)*pPulse)->setFactor(factor);
	if (steps==0)
	{
		if (eAxis==AXIS_GX) ((GradientPulseShape*)*pPulse)->getAreaMethod(1);
		if (eAxis==AXIS_GY) ((GradientPulseShape*)*pPulse)->getAreaMethod(2);
		if (eAxis==AXIS_GZ) ((GradientPulseShape*)*pPulse)->getAreaMethod(3);
	}
 };

/*****************************************************************************/
 void XmlSequence::CreateGradientSpiral(PulseShape** pPulse, DOMNode* node){
	string name="GradientSpiral",item,value;
	double duration=1.0,turns=1.0,tune=0.5,res=1.0;
	PulseAxis eAxis=AXIS_GX;
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	if (pAttributes)
	{
		int nSize = pAttributes->getLength();
		for(int i=0;i<nSize;++i)
		{
			DOMAttr* pAttributeNode = (DOMAttr*) pAttributes->item(i);
			item = XMLString::transcode(pAttributeNode->getName());
			value = XMLString::transcode(pAttributeNode->getValue());
			if (item=="Name")	name = value;
			if (item=="Duration")	duration = atof(value.c_str());
			if (item=="Turns")	turns = atof(value.c_str());
			if (item=="Parameter")	tune = atof(value.c_str());
			if (item=="Resolution")	res = atof(value.c_str());
			if (item=="Axis")
				{
                        		if ( value == "GX" ) eAxis = AXIS_GX ;
                        		if ( value == "GY" ) eAxis = AXIS_GY ;
				}
		}
	}
	*pPulse = new GradientSpiral(duration, turns, tune, res, eAxis, name);
 };

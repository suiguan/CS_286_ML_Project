# CS_286_ML_Project

This is the team project for the class CS 286 (section I) in SJSU for semester Fall 2018. The goal of the team project is to apply advanced machine learning technique to detect Denial of Service (Dos) attacks at the HTTP server application level.

Authors: Deepti Garg & Sui Kun Guan 

Code structure:
- httpServer.py: A python based HTTP server for data collection
- hulk.py: DoS Malware used in the project. Source: https://github.com/GeoffreyVDB/HULK
- httpHeadersData.csv: an example of collected dataset.

- DATA/: 
	- 5-fold.ipynb: a jupyter notebook that creates 5-fold cross validation sequences for malware and benign.
	- createCArr.py: a python script that encodes the malware and benign sequences into C++ syntax so that can be directly hardcoded inside the HMM C++ codes.
	- review-UserAgents.ipynb: a jupyter notebook that exams the user-agent header in the data collected.
	- PlotsHMM.ipynb: a jupyter notebook that plots the HMM scores & PR & ROC curves. 
	- Neural_Network_SVM_5_Fold.ipynb: a jupyter notebook that performance accuracy analysis on combining HMM scores using Neural Network and SVM. 
	- NN_PR_ROC.ipynb : a jupyter notebook that uses a second Neural Network that produce scores instead of classification. It also plots the 5-fold scores and PR & ROC curves. 

- hmm/ : contains an C++ HMM implementation to do 5-fold cross validation with hardcoded malware and benign sequences.

- PCA/:
	- UA_DATA/: Directory contains the 5-fold HMM scores that generated from user-agent for all sequence lengths.
	- PATH_DATA/: Directory contains the 5-fold HMM scores that generated from path for all sequence lengths.
	(Both scores are combined and used in PCA, SVM, and Neural Network analysis.)
	- PCA.ipynb : a jupyter notebook that performs PCA analysis. Scatter plot, and PR Curve. 
  

dash
========

An MPEG/DASH client-server [ns3](https://www.nsnam.org/) module for simulating rate adaptation algorithms over HTTP/TCP.

This module was built for generating the simulation results in the following papers:

  - Dimitrios J. Vergados, Angelos Michalas, Aggeliki Sgora, Dimitrios D. Vergados, and Periklis Chatzimisios. ["FDASH: A Fuzzy-Based MPEG/DASH Adaptation Algorithm."](https://www.researchgate.net/publication/288842567_FDASH_A_Fuzzy-Based_MPEGDASH_Adaptation_Algorithm) IEEE Systems Journal 10, no. 2 (2016): 859-868.
  - Dimitrios J. Vergados, Angelos Michalas, Aggeliki Sgora, and Dimitrios D. Vergados. ["A fuzzy controller for rate adaptation in MPEG-DASH clients."](https://www.researchgate.net/publication/281967962_A_fuzzy_controller_for_rate_adaptation_in_MPEG-DASH_clients) In 2014 IEEE 25th Annual International Symposium on Personal, Indoor, and Mobile Radio Communication (PIMRC), pp. 2008-2012. IEEE, 2014.
  - Dimitrios J. Vergados, Angelos Michalas, Aggeliki Sgora, and Dimitrios D. Vergados. ["A control-based algorithm for rate adaption in MPEG-DASH."](https://www.researchgate.net/publication/268196345_A_control-based_algorithm_for_rate_adaption_in_MPEG-DASH) In Information, Intelligence, Systems and Applications, IISA 2014, The 5th International Conference on, pp. 438-442. IEEE, 2014.
  
The DASH rate adaptation algorithms that are implemented are the following:

  - **FDASH**,  This is the algorithms that is proposed in the above papers
  - **AAASH**, presented in: K. Miller, E. Quacchio, G. Gennari, and A. Wolisz, “Adaptation algorithm for adaptive streaming over HTTP,” in Proc. 19th Int. IEEE PV Workshop, 2012, pp. 173–178. 
  - **OSMF**, presented in: R. K. P. Mok, X. Luo, E. W. W. Chan, and R. K. C. Chang, “QDASH: A QoE-aware DASH system,” in Proc. 3rd MMSys Conf., New York, NY, USA, 2012, pp. 11–22. 
  - **SVAA**, presented in: G. Tian and Y. Liu, “Towards agile and smooth video adaptation in dynamic HTTP streaming,” in Proc. 8th Int. CoNEXT, New York, NY, USA, 2012, pp. 109–120. [Online].
  - **RAAHS**, presented in: C. Liu, I. Bouazizi, and M. Gabbouj, “Rate adaptation for adaptive HTTP streaming,” in Proc. 2nd Annu. ACM Conf. MMSys, New York, NY, USA, 2011, pp. 169–174. [Online].
  - **SFTM**, presented in: C. Liu, I. Bouazizi, M. M. Hannuksela, and M. Gabbouj, “Rate adaptation for dynamic adaptive streaming over HTTP in content distribution network,” Signal Process., Image Commun., vol. 27, no. 4, pp. 288–311, Apr. 2012.

Installation instructions
----

  1. Install ns3 on you system, using the instructions like from here: https://www.nsnam.org/wiki/Installation#Downloading_ns-3_Using_Mercurial.
  2. Download the module into the `src` directory, with the following commands:
       ```
       cd ns-3-dev/
       cd src/
       git clone https://github.com/djvergad/dash.git
       ```
     A new directory named `dash` should be created in the `src` directory   
    
  3. Re-configure ns3 and enable examples. From the `ns-3-dev` directory, type:
       ```
       ./waf configure --enable-examples
       ```

  4. Now the setup is complete. Validate by running an example:  
       ```
       ./waf --run 'src/dash/examples/dash-example --users=3 --protocol="ns3::FdashClient" --linkRate=1000Kbps'
       ```
     There should be output similar to the following:
       ```
       0.829568 Node: 0 newBitRate: 89000 oldBitRate: 45000 estBitRate: 205177 interTime: 0 T: 1.4284 dT: 0 del: +0.0ns
       0.84288 Node: 1 newBitRate: 89000 oldBitRate: 45000 estBitRate: 197645 interTime: 0 T: 1.42024 dT: 0 del: +0.0ns
       0.849848 Node: 2 newBitRate: 89000 oldBitRate: 45000 estBitRate: 185404 interTime: 0 T: 1.41842 dT: 0 del: +0.0ns
       1.40569 Node: 0 newBitRate: 131000 oldBitRate: 89000 estBitRate: 276525 interTime: 0 T: 2.85228 dT: 1.42388 del: +0.0ns
       1.45575 Node: 1 newBitRate: 131000 oldBitRate: 89000 estBitRate: 261727 interTime: 0 T: 2.80737 dT: 1.38713 del: +0.0ns
       1.51679 Node: 2 newBitRate: 89000 oldBitRate: 89000 estBitRate: 246766 interTime: 0 T: 2.75148 dT: 1.33306 del: +0.0ns
       ...
       95.931 Node: 0 newBitRate: 396000 oldBitRate: 396000 estBitRate: 427899 interTime: 0 T: 38.3269 dT: 0.175848 del: +0.0ns
       96.5143 Node: 1 newBitRate: 263000 oldBitRate: 263000 estBitRate: 282589 interTime: 0 T: 37.7488 dT: 0.139504 del: +0.0ns
       96.9279 Node: 2 newBitRate: 178000 oldBitRate: 178000 estBitRate: 197848 interTime: 0 T: 37.3404 dT: 0.12356 del: +0.0ns
       97.7929 Node: 0 newBitRate: 396000 oldBitRate: 396000 estBitRate: 424918 interTime: 0 T: 38.4651 dT: 0.138144 del: +0.0ns
       98.2967 Node: 1 newBitRate: 263000 oldBitRate: 263000 estBitRate: 275899 interTime: 0 T: 37.9664 dT: 0.217576 del: +0.0ns
       98.6872 Node: 2 newBitRate: 178000 oldBitRate: 178000 estBitRate: 200443 interTime: 0 T: 37.581 dT: 0.240664 del: +0.0ns
       99.5082 Node: 0 newBitRate: 396000 oldBitRate: 396000 estBitRate: 426879 interTime: 0 T: 38.7498 dT: 0.284736 del: +0.0ns
       ns3::FdashClient-Node: 0 InterruptionTime: 0 interruptions: 0 avgRate: 248101 minRate: 89000 AvgDt: 38.4632 changes: 7
       ns3::FdashClient-Node: 1 InterruptionTime: 0 interruptions: 0 avgRate: 175772 minRate: 89000 AvgDt: 37.577 changes: 5
       ns3::FdashClient-Node: 2 InterruptionTime: 0 interruptions: 0 avgRate: 120478 minRate: 89000 AvgDt: 36.4311 changes: 3
       ```

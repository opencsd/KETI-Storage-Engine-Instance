## Introduction of OpenCSD KETI-Storage-Engine-Instance
-------------

KEIT-Storage-Engine-Instance for KETI-OpenCSD Platform

Developed by KETI

## Contents
-------------
[1. Requirement](#requirement)

[2. Required Module](#required-Module)

[3. How To Install](#How-To-Install)

[4. Modules](#modules)

[5. Governance](#governance)

## Requirement
-------------
>   Ubuntu 20.04.2 LTS

>   MySQL 5.6(Storage Engine : MyROCKS by KETI-Version)

>   RapidJSON

>   Cmake >= 3.24

>   libprotoc >= 3.19

## Required Module
- *[KETI-DB-Connector-Instance](https://github.com/opencsd/KETI-DB-Connector-Instance)*
- *[KETI-Storage-Engine-Instance](https://github.com/opencsd/KETI-Storage-Engine-Instance)*
- *[KETI-CSD-Proxy](https://github.com/opencsd/KETI-CSD-Proxy)*
- *[KETI-LBA2PBA-Manager](https://github.com/opencsd/KETI-LBA2PBA-Manager)*

## How To Install
-------------
```bash
git clone 
cd KETI-Storage-Engine-Instance/cmake/build
cmake ../..
make -j8
```

## Modules
-------------
### Interface Container
-------------
Be the first to receive and manage snippets to perform queries
#### Storage Engine Input Interface
-------------
Receive snippets for query execution from DB Connector Instance
#### Snippet Manager
-------------
Manage snippet task status by query and deliver snippets to appropriate container

### Monitoring Container
-------------
Manages data related to database schema information required to perform queries
#### LBA2PBA Query Agent
-------------
Communicates with LBA2PBA Manager to convert logical block address to physical block address
#### WAL Query Agent
-------------
Communicates with WAL Manager, which manages unflushed data
#### Table Manager
-------------
Manage database table schema information, etc.
#### Index Manager
-------------
Perform tasks required for index scans, such as index table management and data exploration

### Offloading Container
-------------
Passing snippets to computational storage
#### Snippet Scheduler
-------------
Appropriate compute storage to perform query scan/filter operations is selected according to a scheduling algorithm
#### CSD Status Manager
-------------
State management of computational storage for scheduling

### Merging Container
-------------
Merge and store work results received from computational storage
#### Merge Query Manager
-------------
Merge task results received from computational storage
#### Buffer Manager
-------------
Save merged snippet operation results and query final results


## Governance
-------------
This work was supported by Institute of Information & communications Technology Planning & Evaluation (IITP) grant funded by the Korea government(MSIT) (No.2021-0-00862, Development of DBMS storage engine technology to minimize massive data movement)

## Others
-------------
Due to the structure change, in the first half of the year, we worked on local repositories and private github.
> https://github.com/KETI-OpenCSD/Pushdown-Process-Container

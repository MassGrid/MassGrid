MassGrid dockernode 搭建过程 - 1.3.1.3
=====================================

#### <font color="#dd0000">(注: 本教程截图来自测试网络,适用于主网,配置文件中个别参数不同,文中已有说明)</font> 
## 1. 环境要求

```
操作系统: ubuntu 16.04
推荐配置: 1核、2G内存 、1Mbps带宽 (最低配置：1核、1G内存、1Mbps)
备注:云服务器安全组策略需要添加放行如下TCP端口: 9443,19443,2377,7946
UDP:7946,4789
```  
## 2. 搭建masternode  
详情可见：[masternode 搭建教程](https://github.com/MassGrid/MassGrid/blob/v1.3.1.3/massgrid-docs/masternode-building_zh.md)
## 3. 安装docker  
```
sudo apt-get update
sudo apt-get install -y \
    gnupg-curl \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo apt-key fingerprint 0EBFCD88
sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
sudo apt-get update
sudo apt-get install -y docker-ce=18.06.1~ce~3-0~ubuntu
```
## 4.建立docker swarm集群
```
sudo docker swarm init
```
## 5.配置文件
#### 1) 打开服务端钱包配置文件massgrid.conf   
(默认在 `~/MassGridDataDir/` 目录下，注：`~`表示你的家目录)   

```   
vi ~/MassGridDataDir/massgrid.conf   
```   
添加如下一行:   
```
dockernode=1
```   
##### 2) 设置设备费用
#### 打开服务端钱包配置文件dockerprice.conf 

```   
vi ~/MassGridDataDir/dockerprice.conf  
vi ~/MassGridDataDir/testnet4/dockerprice.conf  //testnet
```   
根据情况设置各种设备价格:  

**设备 型号 单价**
```
cpu i7 0.3
cpu i3 0.2
mem ddr 0.1
gpu nvidia_p106_100_6g 0.3
...
```  




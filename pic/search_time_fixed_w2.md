Comparative analysis of the client’s Search time overheads of VQNomos, Nomos, and Mc-ODXT with |Upd(w2)| fixed to the highest keyword volume.

在这个实验中，需要测试多关键词场景下客户端和服务器和网关的搜索时间开销。实验设置与 Fixed_W1 有一定的区别，这里面也有两个关键词：W1 和 W2。

其中，固定 W2 等于当前数据集下最大的更新次数的关键词；而 W1 则依次取遍当前数据集所有关键词。生成三个数据集下的数据，最后得到三个csv文件，命名格式为{方案名}_{数据集}.csv,例如对于Nomos方案，对于Crime数据集，生成的测试数据文件为Nomos_Crime.csv，生成目录在results/ch4/client_search_time_fixed_w2下。

客户端和服务器和网关的搜索时间开销分别保存到三个目录中，分别为results/ch4/client_search_time_fixed_w2和results/ch4/server_search_time_fixed_w2和results/ch4/gatekeeper_search_time_fixed_w2。
# Multilevel_Indexing
Implementation of Multilevel Indexing in C of 1000+ keys

# Short Details about the project
(More can be seen in ReadMe.txt)

1. There is a binary file **btree.dat** which stores the data(record) address of the data chunk which is stored in secondary memory.

2. As we go on inserting **<Key, Data chunk>** from **binput.dat** in the Btree, the binary file **btree.dat** is updated simultaneously as the pair shift itself between different Nodes.

3. Finally when all the information is inserted, **btree.dat** is completely prepared which contains the Data address of the Data Chunk in the main memory.

4. To search a data, we provide its **key** and get the address corresponding to that key, finally when we get the address, we can process the complete Data Chunk as per our requirement.

5. Btree enhances search of address to **O(logN)** where **N** is the total nodes in Btree.

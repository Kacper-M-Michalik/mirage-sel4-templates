// we use sddf library, which is being built alongside microkit to provide high perforamnce sel4 drivers
// sddf needs to built in spefific ways, which complciates ou build system and system description, 
//link to sddf paper
// tool is provided that uses python to generate system description
// if you run build in buil ddir you will find full.system, but src is python
// implements timer and serial drivers
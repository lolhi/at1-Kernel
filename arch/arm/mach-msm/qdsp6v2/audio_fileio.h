struct file *audiofs_fopen(const char *filename, int flags, int mode);
void audiofs_fclose(struct file *filp);
int audiofs_fseek(struct file *filp, int offset, int whence);
int audiofs_fwrite(struct file *filp, char *buf, int len);

// commented for 'not used' functions
/*int audiofs_ftell(struct file *filp);
int audiofs_fread(struct file *filp, char *buf, int len);
int audiofs_fgetc(struct file *filp);
int audiofs_fgets(struct file *filp, char *str, int size);
void audiofs_fremove(const char *filename);
int audiofs_fputc(struct file *filp, int ch);
int audiofs_fputs(struct file *filp, char *str);
int audiofs_fprintf(struct file *filp, const char *fmt, ...);*/

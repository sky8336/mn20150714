һ��open������ʵ��
hello_open(struct inode *inode,struct file *file)
{
	����ʹ�ü�����������
	���Ϊ��ʼ��������ó�ʼ��
	ʶ����豸�ţ������Ҫ������f_opָ��
	���䲢��д������filp->private_data�����ݽṹ
}
ע�⣺open�����ᱻӦ�ò��open����
      �����в�һ��һ��Ҫдopen�������д�������ϵͳ���Զ����һ��Ĭ�ϵ�open������open������Ҳ����ʲô��������ֱ�ӷ���

����release������ʵ��
hello_release(struct inode *inode,struct file *file)
{
	open�������
}
ע�⣺release�����ᱻӦ�ó����close��������

����read������ʵ��
    read�����ᱻӦ�ò��read��������
hello_read(struct file *file,char __user *buf,size_t count,loff_t *loff)
����2.�û��ռ����ݴ�ŵ�ַ��ָ�봫��
    3.�û�ϣ����ȡ�����ݳ��ȣ���λΪ�ֽ�
��read������ͨ����copy_to_user(buf,data,count)�����ݴ��ں˿ռ�data�и��Ƶ��û��ռ�buf�У���ͨ��buf���͸��û����򣬸ú����ķ���ֵ�Ƚ����⣬��ע�⣺�ɹ�ʱ����0��ʧ��ʱ����һ������

�ġ�write������ʵ��
    write�����ᱻӦ�ò��write��������
hello_write(struct file *file,const char __user *buf,size_t count,loff_t *loff)
����2.�û��ռ���Ҫд������ݴ�ŵ�ַ��ָ�뷽ʽ����
    3.�û�Ҫд������ݳ��ȣ���λΪ�ֽ�
��write������ͨ����copy_from_user(data,buf,count)�����ݴ��û��ռ��buf�и��Ƶ��ں˿ռ��data�У��ú����ķ���ֵ�Ƚ����⣬�ɹ�ʱ����0��ʧ��ʱ����һ������
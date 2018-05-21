#ifndef LOCALSQLCONNECTH
#define LOCALSQLCONNECTH

#ifndef FALSE
    #define FALSE 0
    #define TRUE 1
    #define BOOL int
#endif
//----------------------------------------------------------------------------
struct SqlitePrivate;        //˽������

typedef struct _TSQLiteField
{
	struct SqlitePrivate *Private;
	char Name[16];														//�ֶ���
	int offset;															//�ڼ����ֶ�
    char * (*AsChar)(struct _TSQLiteField *This,char *Buf,int Size);    //��Ϊ�ֶ��Ͷ�ȡ
    int (*AsInt)(struct _TSQLiteField *This);                           //��Ϊ���Ͷ�ȡ
    double (*AsFloat)(struct _TSQLiteField *This);                      //��Ϊ�����Ͷ�ȡ
} TSQLiteField,*PSQLiteField;
//----------------------------------------------------------------------------

typedef struct _TSqlite
{
    struct SqlitePrivate * Private;
	PSQLiteField Fields;
    char ServerIP[16];
    int ServerPort;

    void (*Destroy)(struct _TSqlite *This);        //����
    BOOL (*Open)(struct _TSqlite *This);            //��
    BOOL (*ExecSQL)(struct _TSqlite *This);         //ִ��
    void (*Close)(struct _TSqlite *This);           //�ر�
    void (*First)(struct _TSqlite *This);          //�׼�¼
    void (*Last)(struct _TSqlite *This);           //ĩ��¼
    void (*Prior)(struct _TSqlite *This);          //��һ��¼
    void (*Next)(struct _TSqlite *This);           //��һ��¼
    void (*SetRecNo)(struct _TSqlite *This,int RecNo);       //������¼��
    int (*RecNo)(struct _TSqlite *This);          //���ؼ�¼��
    PSQLiteField (*FieldByName)(struct _TSqlite *This,char *Name);       //�����ֶ�
    int (*GetSQLText)(struct _TSqlite *This,char *pBuf,int Size);  //ȡSQL������
    void (*SetSQLText)(struct _TSqlite *This,char *SqlCmd);        //����SQL������
	BOOL (*Active)(struct _TSqlite *This);							//�Ƿ�򿪱�
    int (*RecordCount)(struct _TSqlite *This);						//������
    int (*FieldCount)(struct _TSqlite *This);						//�ֶ���
	int (*LastRowId)(struct _TSqlite *This);						//ȡ��������Ӱ���ID

    void (*prepare)(struct _TSqlite *This,char *SqlCmd);
    void (*reset)(struct _TSqlite *This);
    void (*finalize)(struct _TSqlite *This);
    void (*step)(struct _TSqlite *This);
    void (*bind_int)(struct _TSqlite *This,int arg);
    void (*bind_text)(struct _TSqlite *This,char *text);
} TSqlite;

TSqlite * CreateLocalQuery(const char *FileName);

BOOL LocalQueryOpen(TSqlite *Query,char *SqlStr);
BOOL LocalQueryExec(TSqlite *Query,char *SqlStr);
char* LocalQueryOfChar(TSqlite *Query,char *FieldName,char *cBuf,int Size);
int LocalQueryOfInt(TSqlite *Query,char *FieldName);
double LocalQueryOfFloat(TSqlite *Query,char *FieldName);
//----------------------------------------------------------------------------
#endif

#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>


enum {
	NOTYPE = 256, EQ, NUM, NEQ, AND, OR, HNU, REG, NEG, POI, MARK

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
	int PRI;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE,0},				// spaces
	{"\\+", '+',4},					// plus
	{"==", EQ,3},						// equal
	{"\\-",'-',4},
	{"\\*",'*',5},					//multiply
	{"/",'/',5},					//divide
	{"\\(",'(',7},
	{"\\)",')',7},
	{"\\b[0-9]+\\b",NUM,0},				//num
	{"\\b0[xX][0-9A-Fa-f]+\\b",HNU,0},		//hex number
	{"!=",NEQ,3},
	{"!",'!',6},
	{"&&",AND,2},
	{"\\|\\|",OR,1},
	{"\\$[a-zA-Z]+",REG,0},
	{"\\b[a-zA-Z_0-9]+",MARK,0}  //mark
	
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
	int PRI;
} Token;

Token tokens[32];
int nr_token;

uint32_t get_addr_from_mark(char * mark);

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE: break;
					case '-':
						/*if there is a '-', judge the previous element is a NUM/')' or not
						 *if it is not a NUM/')', just add a '0' into the experiment
						 */	
							tokens[nr_token].PRI=4;
							tokens[nr_token].str[0]='-';
							tokens[nr_token].str[1]='\0';
							if(nr_token==0||(tokens[nr_token-1].type!=NUM&&tokens[nr_token-1].type!=')'&&tokens[nr_token-1].type!=HNU&&tokens[nr_token-1].type!=REG&& tokens[i - 1].type != MARK)){
								tokens[nr_token].type=NEG;
								tokens[nr_token].PRI=6;
							}
							else tokens[nr_token].type='-';
							nr_token++;
							break;
					/*    
					case NUM:	//a NUM can't follows a ')'
							if(nr_token!=0 && tokens[nr_token-1].type==')'){
								printf("bad expresiion!\n");
								return 0;
							}
								
							tokens[nr_token].type = rules[i].token_type;
							tokens[nr_token].PRI=rules[i].PRI;
							strncpy(tokens[nr_token].str,substr_start,substr_len);
							tokens[nr_token++].str[substr_len]='\0';
							break;
					*/
					case '*':	
							tokens[nr_token].PRI=5;
							tokens[nr_token].str[0]='*';
							tokens[nr_token].str[1]='\0';
							if(nr_token==0||(tokens[nr_token-1].type!=NUM&&tokens[nr_token-1].type!=')'&&tokens[nr_token-1].type!=HNU&&tokens[nr_token-1].type!=REG&& tokens[i - 1].type != MARK)){
								tokens[nr_token].type=POI;
								tokens[nr_token].PRI=6;
							}
							else tokens[nr_token].type='*';
							nr_token++;
							break;

					case REG:	tokens[nr_token].type = rules[i].token_type;				
							tokens[nr_token].PRI=rules[i].PRI;
							strncpy(tokens[nr_token].str,substr_start+1,substr_len-1);
							tokens[nr_token++].str[substr_len-1]='\0';
							break;

					case '(':	
							if(nr_token!=0&&tokens[nr_token-1].type==NUM){
								printf("bad expression 2 !\n");
								return 0;
							}
							// fall into default		
					default: 
						tokens[nr_token].type = rules[i].token_type;
						tokens[nr_token].PRI=rules[i].PRI;
						strncpy(tokens[nr_token].str,substr_start,substr_len);
						tokens[nr_token++].str[substr_len]='\0';
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}
	/*for(i=0;i<nr_token;i++){
		printf("tokens %d, %s, %d\n",tokens[i].type,tokens[i].str,tokens[i].PRI);
	}*/
	//printf("done1\n");

	return true; 
}

int dominant_operator(int p, int q){
	int i, j;
	int op=p;
	int min_PRI = 100;
	for(i = p; i <= q ; i++ ){
		if(tokens[i].type==NUM||tokens[i].type==HNU||tokens[i].type==REG|| tokens[i].type==MARK)
		continue;
		int N = 0;
		int is = 1;
		for(j=i-1;j>=p;j--){
			if(tokens[j].type=='('&&!N){
				is=0;
				break;
			}
			if(tokens[j].type=='(')
				N--;
			if(tokens[j].type==')')
				N++;
		}
		if(!is)continue;
		if(tokens[i].PRI<=min_PRI){
		min_PRI=tokens[i].PRI;
		op=i;
		}
	}
	/*printf("%c",tokens[op].type);
	printf("%d\n",op);*/
	return op;
}

int check_parentheses(int l, int r){
	int i = 0 ;
	int num = 0;
//	printf("%d %d\n", l, r);
	if(tokens[l].type == '(' && tokens[r].type==')'){
		for( i = l+1 ; i <= r-1 ; i++ ){
			if(tokens[i].type=='(')num++;
			if(tokens[i].type==')')num--;
			if(num<0)return -1;
		}
		if(!num){
		//	printf("is\n");
			return 1;
		}
	}
	return 0;
}

int eval(int p, int q){
	if(tokens[p].type==')') {
		printf("bad expression 1 !\n");
		return 0;
	}
	if(p>q) { printf("bad expression 5\n") ;return 0;}
	if(p==q){
		int num = 0;
		if(tokens[p].type==NUM){
		//	printf("a\n");
			sscanf(tokens[p].str,"%d",&num);
		//	printf("b\n");
			return num;
		}
		//TO DO: add hex
		if(tokens[p].type==HNU){
			sscanf(tokens[p].str,"%x",&num);
			return num;
		}
		//rigister
		if(tokens[p].type==REG){
			/*printf("it is a reg\n");
			printf("%s\n",tokens[p].str);
			printf("len = %d \n ",(int) strlen(tokens[p].str));*/
			if(strlen(tokens[p].str)==3){
				int i=0;
				for( i = 0; i<8 ; i++)
					if(!strcmp(tokens[p].str,regsl[i]))break;
					if(i>8)
						if(strcmp(tokens[i].str,"eip")==0)
						num=cpu.eip;
						else Assert(1,"no such register!\n");
					else num=cpu.gpr[i]._32;
			/*printf("i = %d\n", i);
				printf("num = %d\n", num);*/
				
			}
			else if(strlen(tokens[p].str)==2){
				if(tokens[p].str[1]=='x'||tokens[p].str[1]=='p'||tokens[p].str[1]=='i'){
					int i;
					for( i=0 ; i<8 ; i++ )
						if(strcmp(tokens[p].str,regsw[i])==0)break;
					num=cpu.gpr[i]._16;
				}
				else if(tokens[p].str[1]=='l'||tokens[p].str[1]=='h'){

					int i;
					for( i=0 ; i<8 ; i++ )
						if(strcmp(tokens[p].str,regsb[i])==0) break;
					num = cpu.gpr[i]._8[0];
				}
				else Assert(1,"no such register!\n");
			}
			return num;

		}	
		if (tokens[p].type == MARK)
		{
			num = get_addr_from_mark(tokens[p].str);
			printf("%s	", tokens[p].str); 
			return num;
		}
		printf("bad expression 4!\n");
		return 0;
	}
	else if(check_parentheses(p,q)==1)return eval(p+1,q-1);
	/*else if(check_parentheses(p,q)==-1){
		printf("bad expresion! set result = 0 !!!\n");
		return 0;
	}*/
	else {
		//printf("l = %d , r = %d\n", p, q);
		int op = dominant_operator(p,q);
	       	//printf("op = %d\n",op);
		if(op==p||tokens[op].type==POI){
			int value2=eval(op+1,q);
			//printf("value2 = %d\n",value2);
			switch(tokens[op].type){
				case NEG:/*printf("doneNEG\n")*/;return -value2;
				case '!':return !value2;
				case POI:return swaddr_read(value2,4);
			}
		}
		int value1=eval(p,op-1);
		int value2=eval(op+1,q);		
		//printf("%d, %d\n",value1,value2);
		switch(tokens[op].type){
			case '+':return value1+value2;
			case '-':return value1-value2;
			case '*':return value1*value2;
			case '/':return value1/value2;	
			case EQ :return value1==value2;
			case NEQ:return value1!=value2;
			case OR :return value1||value2;
			case AND:return value1&&value2;
			default: break;
		}
	}
	printf("bad expression 3 !\n");
	return 0;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		printf("error!\n");
		return 0;
	}
	*success = 1;
	//printf("done\n");

	/* TODO: Insert codes to evaluate the expression. */
	return eval(0,nr_token-1);
}


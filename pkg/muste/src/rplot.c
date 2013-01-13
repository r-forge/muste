// rplot.c Convert .MOF-file to R-function - RS 5.1.2013
#include "muste.h"
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "survo.h"
#include "survoext.h"
#include "survolib.h"

#define EPS 0.000000001

extern int op_gplot();

int muste_rplotcall;

static void muster_initviewport();
static int muster_line_plot(double x1,double y1,double x2,double y2);
static int muster_curve_plot(int id,double x1,double y1,double x2,double y2,double cx1,double cy1,double cx2,double cy2);
static int muster_rectangle_plot(double x1,double y1,double x2,double y2);
static int muster_polygon_plot(int id,char *chain);	
static int muster_ellipse_plot(double x1,double y1,double x2,double y2);
static int muster_arc_plot(int id,double x1,double y1,double x2,double y2,double a1,double a2);
static int muster_text_plot(double x1,double y1,char *x);

static FILE *mof_playfile;
static FILE *r_outfile;

static int arcincluded;
static int show_rpicture;
static int nextlineread;
static int initnewviewport;
static int xsize,ysize;
static int linewidth,linetype;
static int fontsize,fontface;
static char fontfamily[256];
static char background[16],pencolor[16],charcolor[16];
static char *fillcolor;
static char transparent[]="transparent";
static char *luettu;
static char lukubuffer[2000];
static char *terms[10];
static char xbuffer[LLENGTH];
static char ybuffer[LLENGTH];
static char komento[LLENGTH];
static char abuf[LLENGTH];
static char tbuf[2000];
static char outfilename[LLENGTH];
static char *outfile,*infile,*devfile;

int muste_rplot(char *argv)
	{
	char *teksti,*topaikka;
	int i,device;
	char *devparm[3];

	int plot_id;

	s_init(argv);

	muste_rplotcall=TRUE;

/*
	if (g<3)
		{
		sur_print("\nUsage: RPLOT FILE <file>.MOF TO <Rname>");
		WAIT;
		return(-1);
		}
*/

	i=0;
	if (strcmp(word[g-2],"TO")!=0) strcpy(outfilename,"<Temp>/_RPLOT.R");
	else { strcpy(outfilename,word[g-1]); i=1; }	

	if (strcmp(word[1],"FILE")!=0)
		{
		edread(xbuffer,r1+r-1);
		strcpy(ybuffer,xbuffer);
		teksti=strstr(xbuffer,"RPLOT");
		if (teksti!=NULL) *teksti='G';
		if (i) topaikka=strstr(xbuffer," TO ");
		if (topaikka!=NULL) { *(topaikka+1)='/'; *(topaikka+2)=' '; }
		edwrite(space,r1+r-1,1);
		edwrite(xbuffer,r1+r-1,0);
		muste_dump();   		
		i=op_gplot();
		muste_restore_dump();
		edwrite(space,r1+r-1,1);
		edwrite(ybuffer,r1+r-1,0);
		muste_dump();
		muste_restore_dump();
		if (i<=0) return(-1);
		s_init(argv);
		strcpy(xbuffer,"<Temp>/_RPLOT.MOF");
		infile=xbuffer;				
		}
	else infile=word[1];
	

	i=sp_init(r1+r-1);
	if (i<0) { sur_print("\nToo many specifications!"); WAIT; return(-1); }
	
	strcpy(abuf,infile);
	if (strchr(abuf,'.')==NULL) strcat(abuf,".MOF");
	mof_playfile=muste_fopen(abuf,"rt");
	if (mof_playfile==NULL)
		{
		sprintf(sbuf,"\nCould not open Muste Output File (MOF): %s!",infile);
		PR_EBLD; sur_print(sbuf);
		WAIT; return(-1);
		}

	if (strchr(outfilename,'.')==NULL) strcat(outfilename,".R");
	r_outfile=muste_fopen(outfilename,"wt");
	if (r_outfile==NULL)
		{
		sprintf(sbuf,"\nCould not open output file: %s!",outfilename);
		PR_EBLD; sur_print(sbuf);
		WAIT; return(-1);
		}		

    show_rpicture=1;
    i=spfind("SHOW");
    if (i>=0) show_rpicture=atoi(spb[i]);

	initnewviewport=TRUE;
	xsize=1500; ysize=1500;
	linewidth=1; linetype=0;
	fontsize=12; fontface=1;
	strcpy(fontfamily,"HersheySans");
	strcpy(background,transparent);
	strcpy(pencolor,"#000000");
	strcpy(charcolor,"#000000");
	fillcolor=pencolor;
	nextlineread=FALSE;	
	device=0;
	arcincluded=FALSE;	

	sprintf(komento,"require(grid)");
	fprintf(r_outfile,"%s\n",komento);
	
	while (1)
		{
		if (!nextlineread)
			{
			luettu=fgets(lukubuffer,2000,mof_playfile);
			lukubuffer[strlen(lukubuffer)-1]=EOS;
			}
		if (luettu==NULL) break;
		nextlineread=FALSE;
		if (strncmp(lukubuffer,"text",4)==0)
			{
			teksti=strchr(lukubuffer,'"');
			if (teksti!=NULL) { *teksti=EOS; teksti++; teksti[strlen(teksti)-1]=EOS; }
			}		
			
		i=splitq(lukubuffer,terms,10); if (i<0) break;
		if (strcmp(terms[0],"size")==0 && i==3)
			{ 
			initnewviewport=TRUE; 
			xsize=atoi(terms[1]); 
			ysize=atoi(terms[2]); 
			continue; 
			}
		if (strcmp(terms[0],"charcolor")==0 && i==2) { strncpy(charcolor,terms[1],8); continue; }
		if (strcmp(terms[0],"pencolor")==0 && i==2) { strncpy(pencolor,terms[1],8); fillcolor=pencolor; continue; }
		if (strcmp(terms[0],"linestyle")==0 && i==3) { linetype=atoi(terms[1]); linewidth=atoi(terms[2]); continue; }
		if (strcmp(terms[0],"background")==0 && i==2) { strncpy(background,terms[1],8); continue; }
		if (strcmp(terms[0],"nofill")==0 && i==1) { fillcolor=transparent; continue; }		
		if (strcmp(terms[0],"font")==0 && i==5)
			{ 
			fontsize=atof(terms[1]); 
			if (strcmp(terms[3],"italic")==0) fontface=3;
			else fontface=1;						
			if (strcmp(terms[2],"bold")==0) fontface++;
			
			if (strcmp(terms[4],"Courier")==0) strcpy(fontfamily,"mono");
			else if (strcmp(terms[4],"Mono")==0) strcpy(fontfamily,"mono");
			else if (strcmp(terms[4],"Helvetica")==0) strcpy(fontfamily,"HersheySans");
			else if (strcmp(terms[4],"Swiss")==0) strcpy(fontfamily,"HersheySans");
			else if (strcmp(terms[4],"Arial")==0) strcpy(fontfamily,"HersheySerif");
			else if (strcmp(terms[4],"Times")==0) strcpy(fontfamily,"HersheySerif");
			else strncpy(fontfamily,terms[4],255);				
			}		

// Actual plotting commands			
		if (strcmp(terms[0],"line")==0 && i==5)
			{
			if (initnewviewport) muster_initviewport();			
			i=muster_line_plot(atof(terms[1]),atof(terms[2]),atof(terms[3]),atof(terms[4]));
			if (i) continue; else break;
			}
		if (strcmp(terms[0],"curve")==0 && i==9) { if (initnewviewport) muster_initviewport(); muster_curve_plot(plot_id,atof(terms[1]),atof(terms[2]),atof(terms[3]),atof(terms[4]),atof(terms[5]),atof(terms[6]),atof(terms[7]),atof(terms[8])); continue; }
		if (strcmp(terms[0],"rectangle")==0 && i==5)
			{ 
			if (initnewviewport) muster_initviewport();
			muster_rectangle_plot(atof(terms[1]),atof(terms[2]),atof(terms[3]),atof(terms[4]));
			continue; 
			}
		if (strcmp(terms[0],"ellipse")==0 && i==5)
			{
			if (initnewviewport) muster_initviewport();
			muster_ellipse_plot(atof(terms[1]),atof(terms[2]),atof(terms[3]),atof(terms[4])); 
			continue;
			}
		if (strcmp(terms[0],"arc")==0 && i==7) { if (initnewviewport) muster_initviewport(); muster_arc_plot(plot_id,atof(terms[1]),atof(terms[2]),atof(terms[3]),atof(terms[4]),atof(terms[5]),atof(terms[6])); continue; }
		if (strcmp(terms[0],"text")==0 && i==3) 
			{ 
			if (initnewviewport) muster_initviewport(); 
			muster_text_plot(atof(terms[1]),atof(terms[2]),teksti);
			continue; 
			}
		if (strcmp(terms[0],"polygon")==0 && i==2) { if (initnewviewport) muster_initviewport(); muster_polygon_plot(plot_id,terms[1]); continue; }
		}
	muste_fclose(mof_playfile);
	sprintf(komento,"popViewport()");
	fprintf(r_outfile,"%s\n",komento);		
	muste_fclose(r_outfile);	


	i=spfind("DEVICE");
	if (i>=0)
		{
		strcpy(abuf,spb[i]);
		i=splitq(abuf,devparm,3);
		if (i>0)
			{      	
			device=1;
			muste_strupr(devparm[0]);
			if (strcmp(devparm[0],"PDF")==0) sprintf(komento,"pdf(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"POSTSCRIPT")==0) sprintf(komento,"ps(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"PS")==0) sprintf(komento,"pdf(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"XFIG")==0) sprintf(komento,"xfig(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"BITMAP")==0) sprintf(komento,"bitmap(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"PICTEX")==0) sprintf(komento,"pictex(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"X11")==0) sprintf(komento,"X11(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"CAIRO_PDF")==0) sprintf(komento,"cairo_pdf(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"CAIRO_PS")==0) sprintf(komento,"cairo_ps(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"SVG")==0) sprintf(komento,"svg(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"PNG")==0) sprintf(komento,"png(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"JPEG")==0) sprintf(komento,"jpeg(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"JPG")==0) sprintf(komento,"jpg(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"BMP")==0) sprintf(komento,"bmp(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"TIFF")==0) sprintf(komento,"tiff(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"TIF")==0) sprintf(komento,"tif(\"%s\")",devparm[1]);
			else if (strcmp(devparm[0],"QUARTZ")==0) sprintf(komento,"quartz(\"%s\")",devparm[1]); // Mac OS X only
			else if (strcmp(devparm[0],"METAFILE")==0) sprintf(komento,"win.metafile(\"%s\")",devparm[1]); // Win only
			else if (strcmp(devparm[0],"EMF")==0) sprintf(komento,"metafile(\"%s\")",devparm[1]); // Win only
			else if (strcmp(devparm[0],"WMF")==0) sprintf(komento,"metafile(\"%s\")",devparm[1]); // Win only
			else if (strcmp(devparm[0],"WIN.METAFILE")==0) sprintf(komento,"win.metafile(\"%s\")",devparm[1]); // Win only
			else if (strcmp(devparm[0],"WIN.PRINT")==0) sprintf(komento,"win.print(\"%s\")",devparm[1]); // Win only
			
			else 
				{
				sur_print("\nError with DEVICE!");
				sur_print("\nDo you want to plot to R screen device (Y/N)?");
				i=(char)nextch(""); sprintf(sbuf,"%c",i); sur_print(i);
				if (i!='Y' && i!='y')
					{
					muste_fclose(mof_playfile);
					muste_fclose(r_outfile); 
					return(0); 
					} 
				device=0; show_rpicture=1;	       	
				}
			}
		}
		
		if (show_rpicture || device==1)
			{
			if (device==0) sprintf(komento,"dev.new()");
			muste_evalr(komento);
			muste_expand_path(outfilename); 
			sprintf(abuf,".muste.runsource(\"%s\",echo=FALSE,print.eval=FALSE)",outfilename);
		    muste_evalr(abuf);
		    if (device==1) { sprintf(komento,"dev.off()"); muste_evalr(komento); }
			}
			
		return(1);	
	}

static void muster_initviewport()
	{
	initnewviewport=FALSE;
	sprintf(komento,"pushViewport(viewport(name=\"%s\"))",outfilename);
	fprintf(r_outfile,"%s\n",komento);
	if (strcmp(background,"#FFFFFF")!=0 && strcmp(background,"#ffffff")!=0)
		{	
		sprintf(komento,"grid.rect(width=1,height=1,gp=gpar(fill=\"%s\"))",background);
		fprintf(r_outfile,"%s\n",komento);
		}
	}

static int muster_line_plot(double x1,double y1,double x2,double y2)
	{
	int i,paluu,kierros,cumkierros;
	double oldx,oldy;

	paluu=TRUE;
	oldx=x2; oldy=y2;
	
	sprintf(abuf,"x=c(%g,%g",x1/xsize,x2/xsize);
	strcpy(xbuffer,abuf);
	sprintf(abuf,"y=c(%g,%g",1-(y1/ysize),1-(y2/ysize));
	strcpy(ybuffer,abuf);

	sprintf(komento,"grid.lines(x,y,gp=gpar(col=\"%s\",lwd=%d",pencolor,linewidth);
    switch (linetype)
    	{
// "dashed", "dotted", "dotdash", "longdash", or "twodash" 
// "44",     "13",     "1343",    "73",          "2262"   	
    	case 0: strcat(komento,")"); break;
    	case 1: strcat(komento,",lty=\"73\")"); break;       // long dash
    	case 2: strcat(komento,",lty=\"13\")"); break;       // dotted 
    	case 3: strcat(komento,",lty=\"4314\")"); break;     // dash dotted
    	case 4: strcat(komento,",lty=\"44\")"); break;       // medium dash
    	case 5: strcat(komento,",lty=\"431313\")"); break;   // dash with two dots
    	case 6: strcat(komento,",lty=\"22\")"); break;       // short dash
    	case 7: strcat(komento,",lty=\"43131313\")"); break; // dash with three dots
    	case 8: strcat(komento,",lineend=\"square\")"); break;
    	}

	kierros=2; cumkierros=0;
	while (1)
		{
		luettu=fgets(lukubuffer,2000,mof_playfile);
		if (luettu==NULL) { paluu=FALSE; break; }
		else
			{
			nextlineread=TRUE;
			lukubuffer[strlen(lukubuffer)-1]=EOS;
			if (strncmp(lukubuffer,"line ",5)==0)
				{
				strcpy(tbuf,lukubuffer);
				i=splitq(tbuf,terms,10); if (i!=5) break;
				if ((fabs(oldx-atoi(terms[1]))<EPS) && (fabs(oldy-atof(terms[2]))<EPS))
					{
//Rprintf("\ncont x: %d=%d , y: %d,%d",(int)oldx,atoi(terms[1]),(int)oldy,atoi(terms[2]));					
					kierros++;
					if (kierros>10) { kierros=1; cumkierros++; strcat(xbuffer,"\n"); strcat(ybuffer,"\n"); }					
					oldx=atof(terms[3]); oldy=atof(terms[4]);
					sprintf(abuf,",%g",oldx/xsize);
					strcat(xbuffer,abuf);
					sprintf(abuf,",%g",1-(oldy/ysize));
					strcat(ybuffer,abuf);
					if (cumkierros>100) break;
					}
				else break;
				}
			else break;		
			}
		}
	fprintf(r_outfile,"%s)\n",xbuffer);
	fprintf(r_outfile,"%s)\n",ybuffer);
	fprintf(r_outfile,"%s)\n",komento);	

	return(paluu);
	}

static int muster_curve_plot(int id,double x1,double y1,double x2,double y2,double cx1,double cy1,double cx2,double cy2) 
	{

	sprintf(xbuffer,"x<-c(%g,%g,%g,%g)",x1/xsize,cx1/xsize,cx2/xsize,x2/xsize);
	sprintf(ybuffer,"y<-c(%g,%g,%g,%g)",1-y1/ysize,1-cy1/ysize,1-cy2/ysize,1-y2/ysize);

	sprintf(komento,"grid.bezier(x,y,gp=gpar(col=\"%s\",lwd=%d",pencolor,linewidth);
    switch (linetype)
    	{
// "dashed", "dotted", "dotdash", "longdash", or "twodash" 
// "44",     "13",     "1343",    "73",          "2262"   	
    	case 0: strcat(komento,")"); break;
    	case 1: strcat(komento,",lty=\"73\")"); break;       // long dash
    	case 2: strcat(komento,",lty=\"13\")"); break;       // dotted 
    	case 3: strcat(komento,",lty=\"4314\")"); break;     // dash dotted
    	case 4: strcat(komento,",lty=\"44\")"); break;       // medium dash
    	case 5: strcat(komento,",lty=\"431313\")"); break;   // dash with two dots
    	case 6: strcat(komento,",lty=\"22\")"); break;       // short dash
    	case 7: strcat(komento,",lty=\"43131313\")"); break; // dash with three dots
    	case 8: strcat(komento,",lineend=\"square\")"); break;
    	}
	fprintf(r_outfile,"%s\n",xbuffer);
	fprintf(r_outfile,"%s\n",ybuffer);
	fprintf(r_outfile,"%s)\n",komento);	

	return(0);
	}

static int muster_rectangle_plot(double x1,double y1,double x2,double y2)
	{
	double vaihto,lev,kork;
	
	lev=fabs(x1/xsize-x2/xsize);
	kork=fabs((1-y1)/ysize-(1-y2)/ysize);
	if (x2<x1) { vaihto=x2; x2=x1; x1=vaihto; }
	if (y2<y1) { vaihto=y2; y2=y1; y1=vaihto; }
	sprintf(komento,"grid.rect(x=%g,y=%g,width=%g,height=%g,just=c(\"left\",\"top\")\n,gp=gpar(col=\"%s\",fill=\"%s\"))",x1/xsize,1-y1/ysize,lev,kork,pencolor,fillcolor);
	fprintf(r_outfile,"%s\n",komento);		
	return(0);
	}

static int muster_polygon_plot(int id,char *chain)
	{

    int i,k,n,kierros;
    double pol_point_x[512];
    double pol_point_y[512];
    char *ss[1024];
	
    i=split(chain,ss,1000);
    n=i/2;
    for (k=0; k<n; ++k)
        {
        pol_point_x[k]=(double)atof(ss[2*k])/xsize;
        pol_point_y[k]=(double)(1-atof(ss[2*k+1])/ysize);
        }
	sprintf(abuf,"x=c(");
	strcpy(xbuffer,abuf);
	sprintf(abuf,"y=c(");
	strcpy(ybuffer,abuf);
	kierros=1;	   
    for (k=0; k<n; k++)
    	{
		kierros++;
		if (kierros>10) { kierros=1; strcat(xbuffer,"\n"); strcat(ybuffer,"\n"); }					
    	sprintf(abuf,"%g,",pol_point_x[k]);
    	strcat(xbuffer,abuf);    	
    	sprintf(abuf,"%g,",pol_point_y[k]);
    	strcat(ybuffer,abuf); 
    	}
	xbuffer[strlen(xbuffer)-1]=')';
	ybuffer[strlen(ybuffer)-1]=')';
	
	sprintf(komento,"grid.polygon(x,y,gp=gpar(col=\"%s\",fill=\"%s\",lwd=%d))",pencolor,fillcolor,linewidth);    

	fprintf(r_outfile,"%s\n",xbuffer);
	fprintf(r_outfile,"%s\n",ybuffer);
	fprintf(r_outfile,"%s\n",komento);	

	return(0);
	}

	
static int muster_ellipse_plot(double x1,double y1,double x2,double y2)
	{
/*	
ellipse <- function (x=0,y=0,a=1,b=1,an=pi/3,n=300) {
 cc <- exp(seq(0,n)*(0+2i)*pi/n)
 R <- matrix(c(cos(an),sin(an),-sin(an),cos(an)),ncol=2,byrow=T)
 res <- cbind(x=a*Re(cc),y=b*Im(cc))%*%R
 list(unit(x,"npc")+unit(res[,1],"snpc"),unit(y,"npc")+unit(res[,2],"snpc")) }


a <- 0.45
b <- 0.60
e<-ellipse(0.5, 0.5, a=a, b=b,an=pi/3)
grid.polygon(e[[1]],e[[2]])	
*/

	sprintf(komento,"grid.circle(%g,%g,r=%g,gp=gpar(col=\"%s\",fill=\"%s\"))",
		((x1+x2)/2)/xsize,1-((y1+y2)/2)/ysize,fabs((x1-x2)/xsize/2),pencolor,fillcolor);
	fprintf(r_outfile,"%s\n",komento);
	
	return(0);
	}	

static int muster_arc_plot(int id,double x1,double y1,double x2,double y2,double a1,double a2)
	{
	if (!arcincluded)
		{
		strcpy(abuf,"arc <- function(x=0,y=0,r=1,a0=0,a1=2*pi,n=100) { ra <- seq(a0,a1,length=n)\n");
		strcat(abuf,"list(unit(x,\"npc\")+unit(c(0,cos(ra)*r,0),\"snpc\"),unit(y,\"npc\")+unit(c(0,sin(ra)*r,0),\"snpc\")) }");
		fprintf(r_outfile,"%s\n",abuf);
		arcincluded=TRUE;
		}
	sprintf(komento,"a<-arc(%g,%g,%g,%g,%g)",
		((x1+x2)/2)/xsize,1-((y1+y2)/2)/ysize,fabs((x1-x2)/xsize/2),a1*PI/180,(a1+a2)*PI/180);
	fprintf(r_outfile,"%s\n",komento);	
	sprintf(komento,"grid.polygon(a[[1]],a[[2]],gp=gpar(fill=\"%s\"))",fillcolor);
	fprintf(r_outfile,"%s\n",komento);	
/*	
arc <- function(x=0,y=0,r=1,a0=0,a1=2*pi1,n=100) { ra <- seq(a0,a1,length=n)
 list(unit(x,"npc")+unit(c(0,cos(ra)*r,0),"snpc"),unit(y,"npc")+unit(c(0,sin(ra)*r,0),"snpc")) }

aa<-arc(0.5,0.5,0.7,0,pi*(3/2))
grid.polygon(aa[[1]],aa[[2]])
*/	
/*	
	double xkerroin,ykerroin;
	xkerroin=(double)((double)muste_x_wsize/(double)muste_x_size);
	ykerroin=(double)((double)muste_y_wsize/(double)muste_y_size);	
	x1*=xkerroin; x2*=xkerroin;
	y1*=ykerroin; y2*=ykerroin;
 
    sprintf(komento,"create arc %g %g %g %g -extent %g -start %g -fill %s",x1,y1,x2,y2,a2,a1,muste_pencolor);
    muste_plottcl(id, komento, FALSE);
*/
	return(0);
	}	
		
static int muster_text_plot(double x1,double y1,char *x)
	{

    char y[2*LLENGTH];
    int i,j;
/*
// RS Handle special characters: 34=" 92=\    
    for (i=0, j=0; i<strlen(x)-1; i++)
    	{
    	if ((unsigned char)x[i]>31) // RS Handle only printable characters
       		{
			if (x[i]==34 || x[i]==39) y[j++]=92;
      		y[j++]=x[i];
      		}
      	else y[j++]=' ';
     }

    if (y[j-1]=='"') y[j++]=' ';
    y[j]=EOS;
*/
	strcpy(y,x);
	muste_iconv(y,"","CP850");
	sprintf(komento,"grid.text(\"%s\",x=%g,y=%g\n,just=c(\"left\",\"top\"),gp=gpar(col=\"%s\",fontface=%d,fontsize=%d,fontfamily=\"%s\"))",y,x1/xsize,1-y1/ysize,charcolor,fontface,fontsize,fontfamily);	  
	fprintf(r_outfile,"%s\n",komento);
	
	return(0);
	}

// C++ Interface: 
//
// Description: 
//
//
//
// Copyright: See COPYING file that comes with this distribution
//
//


#include <QFileDialog>

#include "ADM_default.h"
#include "ADM_toolkitQt.h"

#include "DIA_fileSel.h"
#include "DIA_coreToolkit.h"
#include "DIA_coreUI_internal.h"

#include "prefs.h"
#include "ADM_last.h"
#include "ADM_script2/include/ADM_script.h"

extern QWidget *QuiMainWindows;
static IScriptEngine *tempEngine;

static QWidget *fileSelGetParent(void)
{
    QWidget *parent=qtLastRegisteredDialog();
    if(!parent)
        parent=QuiMainWindows;
    return parent;
}

#ifdef __APPLE__
    #define MACOS_MENU_BAR_STEALS_KEYBOARD_SHORTCUTS
    #include "Q_gui2.h"
#endif

static uint32_t disableMenus(void)
{
    uint32_t menuState = 0;
#ifdef MACOS_MENU_BAR_STEALS_KEYBOARD_SHORTCUTS
    MainWindow *mw = (MainWindow *)QuiMainWindows;
    uint8_t shift = 0;
#define DOIT(x) if (mw->ui.menu##x->isEnabled()) { menuState |= 1 << shift++; }
    DOIT(File)
    DOIT(Recent)
    DOIT(Edit)
    DOIT(View)
    DOIT(Video)
    DOIT(Audio)
    DOIT(Auto)
    DOIT(Tools)
    DOIT(Go)
    DOIT(Custom)
    DOIT(Help)
#undef DOIT
    shift = 0;
#define DOIT(x) if (menuState & (1 << shift++)) { mw->ui.menu##x->setEnabled(false); }
    DOIT(File)
    DOIT(Recent)
    DOIT(Edit)
    DOIT(View)
    DOIT(Video)
    DOIT(Audio)
    DOIT(Auto)
    DOIT(Tools)
    DOIT(Go)
    DOIT(Custom)
    DOIT(Help)
#undef DOIT
#endif
    return menuState;
}

static void enableMenus(uint32_t menuState)
{
#ifdef MACOS_MENU_BAR_STEALS_KEYBOARD_SHORTCUTS
    MainWindow *mw = (MainWindow *)QuiMainWindows;
    uint8_t shift = 0;
#define DOIT(x) if (menuState & (1 << shift++)) { mw->ui.menu##x->setEnabled(true); }
    DOIT(File)
    DOIT(Recent)
    DOIT(Edit)
    DOIT(View)
    DOIT(Video)
    DOIT(Audio)
    DOIT(Auto)
    DOIT(Tools)
    DOIT(Go)
    DOIT(Custom)
    DOIT(Help)
#undef DOIT
#else
    UNUSED_ARG(menuState);
#endif
}

/**
 * \fn fileSelWriteInternal
 */
static int fileSelWriteInternal(const char *label, char *target, uint32_t max, const char *location, const char *ext)
{
    QString str, outputPath, fileName, outputExt;
    QString separator = "/"; // Qt uses forward slash on Windows too
    QString filterFile=QString::fromUtf8(QT_TRANSLATE_NOOP("qfile","All files (*.*)"));
    QFileDialog::Options opts = QFileDialog::Options();
    bool doFilter = !!(ext && strlen(ext));
    bool isProject=false;
    int extSize=1;

    if(doFilter)
    {
        outputExt = ".";
        outputExt += ext;
        extSize+=strlen(ext);
        for(int i=0; i < getScriptEngines().size(); i++)
        {
            tempEngine = getScriptEngines()[i];
            std::string dext = tempEngine->defaultFileExtension();
            if(!dext.empty() && dext == ext)
            {
                isProject=true;
                break;
            }
        }
    }

    if(location)
        outputPath = QFileInfo(location).path();

    if(!location || !QDir(outputPath).exists())
    {
        bool lastReadAsTarget=false;
        prefs->get(FEATURES_USE_LAST_READ_DIR_AS_TARGET,&lastReadAsTarget);
        std::string lastFolder;
        if(!lastReadAsTarget)
        {
            if(!isProject)
            {
                admCoreUtils::getLastWriteFolder(lastFolder);
            }else
            {
                admCoreUtils::getLastProjectWriteFolder(lastFolder);
            }
            if(!lastFolder.size())
            {
                if(!isProject)
                {
                    admCoreUtils::getLastReadFolder(lastFolder);
                }else
                {
                    admCoreUtils::getLastProjectReadFolder(lastFolder);
                }
            }
        }else
        {
            admCoreUtils::getLastReadFolder(lastFolder);
        }

        if(lastFolder.size())
        {
            outputPath = QFileInfo(QString::fromUtf8(lastFolder.c_str())).path();
        }
    }

    /* LASTDIR may have gone; then use the user's homedir instead */
    if(outputPath.isEmpty() || !QDir(outputPath).exists())
        outputPath = QDir::homePath();

    QString inputBaseName;
    std::string lastRead;
    admCoreUtils::getLastReadFile(lastRead);
    if(lastRead.size())
    {
        inputBaseName = QFileInfo(QString::fromUtf8(lastRead.c_str())).completeBaseName();
        str = outputPath+separator+inputBaseName+outputExt;
        char *canonicalNew = ADM_PathCanonize(str.toUtf8().constData());
        char *canonicalLast = ADM_PathCanonize(lastRead.c_str());

        if(!strcmp(canonicalNew, canonicalLast))
        { // try to avoid name collision when saving in the same directory as the currently loaded video
            str = outputPath;
            str += separator;
            str += inputBaseName;
            str += "_edit";
            str += outputExt;
        }
        delete [] canonicalNew;
        delete [] canonicalLast;
    }else
    {
        str = QDir::homePath();
        str += separator;
        str += "out";
        str += outputExt;
    }

    if(doFilter)
    {
        QString prependTo = ext;
        prependTo += QString::fromUtf8(QT_TRANSLATE_NOOP("qfile"," files (*."));
        prependTo += ext;
        prependTo += ");;";
        filterFile = prependTo + filterFile;
    }

#ifndef __APPLE__
    opts = QFileDialog::DontConfirmOverwrite; // doesn't work on macOS, wtf?
#endif

    uint32_t menuState = disableMenus();

    fileName = QFileDialog::getSaveFileName(fileSelGetParent(),
                    QString::fromUtf8(label),  // caption
                    str,    // folder
                    filterFile,   // filter
                    NULL, // selected filter
                    opts);

#ifdef _WIN32
    fileName = QDir::toNativeSeparators(fileName);
#endif

    enableMenus(menuState);

    int len = strlen(fileName.toUtf8().constData());
    if(!len || len >= max) return 0;

    // Check if we need to add an extension....
    if(doFilter)
    {
        QString s = ext;
        if(QFileInfo(fileName).suffix().toLower() != s.toLower())
        {
            fileName += ".";
            fileName += ext;
            len+=extSize;
        }
    }

    if(opts & QFileDialog::DontConfirmOverwrite)
    {
        QFile newFile(fileName);
        if(newFile.exists())
        {
            QFileInfo fileInfo(newFile);
            char *format = ADM_strdup(QT_TRANSLATE_NOOP("qfile", "Overwrite file \"%s\"?"));
            ADM_assert(format);

            uint32_t mxlen = strlen(format) + max + 1;
            char *buf = (char *)ADM_alloc(mxlen);
            snprintf(buf, mxlen, format, fileInfo.fileName().toUtf8().constData());
            // Show the dialog even in silent mode or if the user has disabled alerts.
            bool cancel = (ADM_ERR == GUI_Question(buf, true));
            ADM_dealloc(format);
            ADM_dealloc(buf);
            format = NULL;
            buf = NULL;
            if (cancel)
                return 0;
        }
    }

    if(len >= max)
    {
        ADM_warning("Path length %d exceeds max %d\n",len,max-1);
        return 0;
    }

    strncpy(target,fileName.toUtf8().constData(),len);
    target[len]='\0';

    if(!isProject)
    {
        admCoreUtils::setLastWriteFolder( std::string(fileName.toUtf8().constData()));
    }else
    {
        admCoreUtils::setLastProjectWriteFolder( std::string(fileName.toUtf8().constData()));
    }
    return len;
}


/**
 * \fn fileSelReadInternal
 */
static int fileSelReadInternal(const char *label, char *target, uint32_t max, const char *location, const char *ext)
{
    QString str;
    QString fileName;
    QString filterFile=QString::fromUtf8(QT_TRANSLATE_NOOP("qfile","All files (*.*)"));
    bool doFilter = !!(ext && strlen(ext));
    bool isProject=false;
    QFileDialog::Options opts = QFileDialog::Options();

    if(doFilter)
    {
        for(int i=0; i < getScriptEngines().size(); i++)
        {
            tempEngine = getScriptEngines()[i];
            std::string dext = tempEngine->defaultFileExtension();
            if(!dext.empty() && dext == ext)
            {
                isProject=true;
                break;
            }
        }
    }

    if(location)
        str = QFileInfo(location).path();

    if(!location || !QDir(str).exists())
    {
        std::string lastFolder;
        if(!isProject)
        {
            admCoreUtils::getLastReadFolder(lastFolder);
        }else
        {
            admCoreUtils::getLastProjectReadFolder(lastFolder);
        }

        if (lastFolder.size())
        {
            str = QFileInfo(QString::fromUtf8(lastFolder.c_str())).path();
            /* LASTDIR may have gone; then use the user's homedir instead */
            if (!QDir(str).exists())
                str = QDir::homePath();
        }else
        {
            str = QDir::homePath();
        }
    }

    if(doFilter)
    {
        QString prependTo = ext;
        prependTo += QString::fromUtf8(QT_TRANSLATE_NOOP("qfile"," files (*."));
        prependTo += ext;
        prependTo += ");;";
        filterFile = prependTo + filterFile;
    }

    uint32_t menuState = disableMenus();

    fileName = QFileDialog::getOpenFileName(fileSelGetParent(),
                                QString::fromUtf8(label),  // caption
                                str,    // folder
                                filterFile,   // filter
                                NULL,   // selected filter
                                opts);

#ifdef _WIN32
    fileName = QDir::toNativeSeparators(fileName);
#endif

    enableMenus(menuState);

    int len = strlen(fileName.toUtf8().constData());
    if(!len || len >= max) return 0;

    strncpy(target,fileName.toUtf8().constData(),len);
    target[len]='\0';

    if(!isProject)
    {
        admCoreUtils::setLastReadFolder(std::string(fileName.toUtf8().constData()));
    }else
    {
        admCoreUtils::setLastProjectReadFolder(std::string(fileName.toUtf8().constData()));
    }
    return len;
}        

/*****************************************************/

namespace ADM_QT4_fileSel
{

#if defined(__APPLE__)
 #define MAX_LEN 1024
#else
 #define MAX_LEN 4096
#endif
void GUI_FileSelRead(const char *label, char **name)
{
    char *fn=(char *)ADM_alloc(MAX_LEN);
    if(fileSelReadInternal(label,fn,MAX_LEN,NULL,NULL))
        *name=ADM_strdup(fn);
    else
        *name=NULL;
    ADM_dealloc(fn);
}

void GUI_FileSelWrite(const char *label, char **name)
{
    char *fn=(char *)ADM_alloc(MAX_LEN);
    if(fileSelWriteInternal(label,fn,MAX_LEN,NULL,NULL))
        *name=ADM_strdup(fn);
    else
        *name=NULL;
    ADM_dealloc(fn);
}

void GUI_FileSelRead(const char *label, SELFILE_CB cb)
{
        char *name;

        GUI_FileSelRead(label, &name);

        if (name)
        {
                cb(name); 
                ADM_dealloc(name);
        }
}        
/**
 * \fn GUI_FileSelWrite
 * @param label
 * @param cb
 */
void GUI_FileSelWrite(const char *label, SELFILE_CB cb)
{
        char *name;

        GUI_FileSelWrite(label, &name);

        if (name)
        {
                cb(name); 
                ADM_dealloc(name);
        }
}

/**
          \fn FileSel_SelectWrite
          \brief select file, write mode
          @param title window title
          @param target where to store result
          @param max Max buffer size in bytes
          @param source Original value
          @param extension Filter for the view
          @return 1 on success, 0 on failure
*/
uint8_t FileSel_SelectWrite(const char *title, char *target, uint32_t max, const char *source, const char *extension)
{
    if(fileSelWriteInternal(title,target,max,source,extension))
        return 1;
    return 0;
}

/**
          \fn FileSel_SelectRead
          \brief select file, read mode
          @param title window title
          @param target where to store result
          @param max Max buffer size in bytes
          @param source Original value
          @param extension Filter for the view
          @return 1 on success, 0 on failure
*/
uint8_t FileSel_SelectRead(const char *title, char *target, uint32_t max, const char *source, const char *extension)
{
    if(fileSelReadInternal(title,target,max,source,extension))
        return 1;
    return 0;
}

/**
          \fn FileSel_SelectDir
          \brief select directory
          @param title window title
          @param target where to store result
          @param max Max buffer size in bytes
          @param source Original value
          @return 1 on success, 0 on failure
*/
uint8_t FileSel_SelectDir(const char *title, char *target, uint32_t max, const char *source, const char *extension)
{
        QString fileName;
        QFileDialog::Options options = QFileDialog::ShowDirsOnly;

        fileName = QFileDialog::getExistingDirectory(fileSelGetParent(), title, source, options);

        if (!fileName.isNull())
        {
                const char *s = fileName.toUtf8().constData();
                strncpy(target, s, max);

                return 1;
        }

        return 0;
}
void GUI_FileSelWriteExtension(const char *label, const char *extension,SELFILE_CB cb)
{
    char *name=NULL;
    char *fn=(char *)ADM_alloc(MAX_LEN);
    if(fileSelWriteInternal(label,fn,MAX_LEN,NULL,extension))
        name=ADM_strdup(fn);
    ADM_dealloc(fn);
    if(name)
    {
        cb(name);
        ADM_dealloc(name);
    }
}
void GUI_FileSelReadExtension(const char *label, const char *extension,SELFILE_CB cb)
{
    char *name=NULL;
    char *fn=(char *)ADM_alloc(MAX_LEN);
    if(fileSelReadInternal(label,fn,MAX_LEN,NULL,extension))
        name=ADM_strdup(fn);
    ADM_dealloc(fn);
    if(name)
    {
        cb(name);
        ADM_dealloc(name);
    }
}        
/**
         * 
*/
void init(void)
{
        // Nothing special to do for QT4 fileselector
}
} // End of nameSpace

static DIA_FILESEL_DESC_T Qt4FileSelDesc =
{
        ADM_QT4_fileSel::init,
        ADM_QT4_fileSel::GUI_FileSelRead,
        ADM_QT4_fileSel::GUI_FileSelWrite,
        ADM_QT4_fileSel::GUI_FileSelRead,
        ADM_QT4_fileSel::GUI_FileSelWrite,
        ADM_QT4_fileSel::FileSel_SelectRead,
        ADM_QT4_fileSel::FileSel_SelectWrite,
        ADM_QT4_fileSel::FileSel_SelectDir,
        ADM_QT4_fileSel::GUI_FileSelWriteExtension,
        ADM_QT4_fileSel::GUI_FileSelReadExtension
};

// Hook our functions
void initFileSelector(void)
{
        DIA_fileSelInit(&Qt4FileSelDesc);
}


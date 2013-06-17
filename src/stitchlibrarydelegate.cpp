/*************************************************\
| Copyright (c) 2011 Stitch Works Software        |
| Brian C. Milco <brian@stitchworkssoftware.com>  |
\*************************************************/
#include "stitchlibrarydelegate.h"

#include "stitch.h"
#include <QPainter>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

#include <QStyleOption>
#include <QStyleOptionViewItem>
#include <QSortFilterProxyModel>
#include <QModelIndex>

#include <math.h>

#include <QApplication>

#include "settings.h"
#include <QFileInfo>
#include <QDir>

#include "debug.h"
#include "stitchlibrary.h"
#include "stitchset.h"
#include <QSvgRenderer>
#include <QMessageBox>
#include <QCheckBox>

#include <QMouseEvent>

/***************************************************************************************************************/
static QRect CheckBoxRect(const QStyleOptionViewItem &itemStyleOptions) {
    QStyleOptionButton styleOptions;
    QRect rect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &styleOptions);
    QPoint check_box_point(itemStyleOptions.rect.x() + itemStyleOptions.rect.width() / 2 - rect.width() / 2,
                           itemStyleOptions.rect.y() + itemStyleOptions.rect.height() / 2 - rect.height() / 2);
    return QRect(check_box_point, rect.size());
}
/***************************************************************************************************************/

StitchLibraryDelegate::StitchLibraryDelegate(QWidget* parent)
    : QStyledItemDelegate(parent)
{
}

void StitchLibraryDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(!index.isValid())
        return;

    const QSortFilterProxyModel *model =  static_cast<const QSortFilterProxyModel*>(index.model());
    QModelIndex idx = model->mapToSource(model->index(index.row(), 0));

    //Icon column:
    if (index.column() == 1) {
        if(option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else if(option.state & QStyle::State_MouseOver)
            painter->fillRect(option.rect, option.palette.highlight().color().light(190));

        QRectF rect = QRectF((qreal)option.rect.x(), (qreal)option.rect.y(),
                             (qreal)option.rect.width(), (qreal)option.rect.height());

        Stitch *s = static_cast<Stitch*>(idx.internalPointer());
        if(!s)
            return;

        if(s->width() < rect.width())
            rect.setWidth(s->width());
        if(s->height() < rect.height())
            rect.setHeight(s->height());

        if(s->isSvg()) {
            QSvgRenderer *r = s->renderSvg();
            if(r)
                r->render(painter, rect);

        } else {
            QPixmap *pix = s->renderPixmap();
            if(pix)
                painter->drawPixmap(rect.toRect(), *pix);
        }

    //Checkbox column:
    } else if(index.column() == 5) {
        if(option.state & QStyle::State_MouseOver)
            painter->fillRect(option.rect, option.palette.highlight().color().light(190));

        bool checked = idx.model()->data(idx, Qt::DisplayRole).toBool();
        
        QStyleOptionButton styleOptions;
        styleOptions.state |= QStyle::State_Enabled;
        if(checked)
            styleOptions.state |= QStyle::State_On;
        else
            styleOptions.state |= QStyle::State_Off;
        
        styleOptions.rect = CheckBoxRect(option);
        
        qApp->style()->drawControl(QStyle::CE_CheckBox, &styleOptions, painter);

    //Everything else:
    } else {
        //fall back to the basic painter.
        QStyledItemDelegate::paint(painter, option, idx);
    }
}

QSize StitchLibraryDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int padding = 0;

    if(!index.isValid())
        return QSize(100, 32);

    const QSortFilterProxyModel *model =  static_cast<const QSortFilterProxyModel*>(index.model());
    QModelIndex idx = model->mapToSource(model->index(index.row(), 0));

    Stitch *s = static_cast<Stitch*>(idx.internalPointer());
    if(!s)
        return QSize(100, 32);

    QString text;

    switch(idx.column()) {
        case Stitch::Name:
            text = s->name();
            padding += 50;
            break;
        case Stitch::Icon: {
            QSize retSize;

            if(s->isSvg()) {
                QSvgRenderer *r = s->renderSvg();
                if(r)
                    retSize = r->defaultSize();
                else
                    retSize = QSize(64, 64);
            } else {
                QPixmap *p = s->renderPixmap();
                if(p)
                    retSize = p->size();
                else
                    retSize = QSize(64, 64);
            }
            return retSize;
        }
        case Stitch::Description:
            padding +=150;
            text = s->description();
            break;
        case Stitch::Category:
            padding += 50;
            text = s->category();
            break;
        case Stitch::WrongSide:
            padding +=50;
            text = s->wrongSide();
            break;
        case 5:
            padding += 50;
            text = ""; //TODO: estimate the size of a checkbox.
            break;
        default:
            text = "";
            break;
    }

    QSize hint = option.fontMetrics.size(Qt::TextWordWrap, text);
    hint.setWidth(hint.width() + padding);

    //HACK: make the height of the icon the height of the whole row.
    StitchSet *set = static_cast<StitchSet*>((QAbstractItemModel*)idx.model());
    QSize sizeH = sizeHint(option, set->index(idx.row(), Stitch::Icon));
    hint.setHeight(sizeH.height());

    return hint;
}

QWidget* StitchLibraryDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    if(!index.isValid())
        return new QWidget(parent);

    switch(index.column()) {
        case Stitch::Name:{
            QLineEdit *editor = new QLineEdit(parent);
            //QRegExpValidator* validator = new QRegExpValidator(QRegExp("[a-zA-Z][a-zA-Z0-9]{,}"), editor);
            //editor->setValidator(validator);
            return editor;
        }
        case Stitch::Icon: {
            IconComboBox *cb = new IconComboBox(parent);
            loadIcons(cb);
            cb->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            cb->setIconSize(QSize(32,32));
            cb->model()->sort(0); //TODO: check case sensitivity to the sorting.
            return cb;
        }
        case Stitch::Description: {
            QLineEdit *editor = new QLineEdit(parent);
            return editor;
        }
        case Stitch::Category: {
            QComboBox *cb = new QComboBox(parent);
            cb->addItems(StitchLibrary::inst()->categoryList());
            return cb;
        }
        case Stitch::WrongSide: {
            QComboBox *cb = new QComboBox(parent);
            cb->addItems(StitchLibrary::inst()->stitchList(true));
            return cb;
        }
        case 5: {
            QCheckBox *cb = new QCheckBox(parent);
            return cb;
        }
        default:
            return new QWidget(parent);
    }

    return new QWidget(parent);
}

void StitchLibraryDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if(!index.isValid())
        return;

    const QSortFilterProxyModel *model =  static_cast<const QSortFilterProxyModel*>(index.model());
    QModelIndex idx = model->mapToSource(model->index(index.row(), 0));

    switch(index.column()) {
        case Stitch::Name: {
            QLineEdit* le = static_cast<QLineEdit*>(editor);
            le->setText(idx.data(Qt::EditRole).toString());
            break;
        }
        case Stitch::Icon: {
            IconComboBox* cb = static_cast<IconComboBox*>(editor);
            cb->setCurrentIndex(cb->findData(idx.data(Qt::EditRole), Qt::UserRole));
            break;
        }
        case Stitch::Description: {
            QLineEdit* le = static_cast<QLineEdit*>(editor);
            le->setText(idx.data(Qt::EditRole).toString());
            break;
        }
        case Stitch::Category: {
            QComboBox* cb = static_cast<QComboBox*>(editor);
            cb->setCurrentIndex(cb->findText(idx.data(Qt::EditRole).toString()));
            break;
        }
        case Stitch::WrongSide: {
            QComboBox* cb = static_cast<QComboBox*>(editor);
            cb->setCurrentIndex(cb->findText(idx.data(Qt::EditRole).toString()));
            break;
        }
        case 5: {
            QCheckBox* cb = static_cast<QCheckBox*>(editor);
            cb->setChecked(idx.data(Qt::EditRole).toBool());
            break;
        }
        default:
            break;
    }
}

void StitchLibraryDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSortFilterProxyModel *m = static_cast<QSortFilterProxyModel*>(model);
    QModelIndex idx = m->mapToSource(m->index(index.row(), 0));

    StitchSet *set = static_cast<StitchSet*>(m->sourceModel());

    if(!set) {
        qWarning() << "setModelData: no set found";
        return;
    }

    switch(index.column()) {
        case Stitch::Icon: {
            IconComboBox* cb = static_cast<IconComboBox*>(editor);
            m->setData(idx, cb->itemData(cb->currentIndex(), Qt::UserRole), Qt::EditRole);
            break;
        }
        case Stitch::Name: {
            QLineEdit *le = static_cast<QLineEdit*>(editor);

            Stitch *s = static_cast<Stitch*>(index.internalPointer());
            Stitch *found = set->findStitch(le->text());

            //is there a stitch with the new name in this set already?
            if(found && found != s) {
                QMessageBox msgbox;
                //TODO: return to the editor with the bad data.
                msgbox.setText(tr("A stitch with this name already exists in the set."));
                msgbox.setIcon(QMessageBox::Warning);
                msgbox.exec();

                break;
            }

            //is this stitch in the master list? if so is there a stitch with the new name already?
            found = 0;
            found = StitchLibrary::inst()->masterStitchSet()->findStitch(s->name());
            if(found && found == s) {
                Stitch *match = 0;
                match = StitchLibrary::inst()->masterStitchSet()->findStitch(le->text());
                if(match && match != s) {
                    QMessageBox msgbox;
                    msgbox.setText("There is already a stitch with this name in the master list");
                    msgbox.setIcon(QMessageBox::Warning);
                    msgbox.exec();
                    //TODO: offer to remove the stitch already there with this name.

                    break;
                }
            }
                
            m->setData(idx, le->text(), Qt::EditRole);
            
            break;
        }
        case Stitch::Description: {
            QLineEdit* le = static_cast<QLineEdit*>(editor);
            m->setData(idx, le->text(), Qt::EditRole);

            break;
        }
        case Stitch::WrongSide:
        case Stitch::Category: {
            QComboBox* cb = static_cast<QComboBox*>(editor);
            m->setData(idx, cb->currentText(), Qt::EditRole);
            break;
        }
        case 5: {
            QCheckBox* cb = static_cast<QCheckBox*>(editor);
            m->setData(idx, cb->isChecked(), Qt::EditRole);
            break;
        }
        default:
            break;
    }
}

void StitchLibraryDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);

    editor->setGeometry(option.rect);
}

bool StitchLibraryDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    QSortFilterProxyModel *m = static_cast<QSortFilterProxyModel*>(model);
    QModelIndex idx = m->mapToSource(m->index(index.row(), 0));

    //checkbox
    if(index.column() == 5) {
        if ((event->type() == QEvent::MouseButtonRelease) || (event->type() == QEvent::MouseButtonDblClick)) {
            QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);

            if (mouse_event->button() != Qt::LeftButton || !CheckBoxRect(option).contains(mouse_event->pos())) {
                return false;
            }
            if (event->type() == QEvent::MouseButtonDblClick) {
                return true;
            }
        } else if (event->type() == QEvent::KeyPress) {
            if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space &&
                    static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select) {
                return false;
            }
        } else {
            return false;
        }
            
        bool checked = idx.model()->data(idx, Qt::DisplayRole).toBool();
        return m->setData(idx, !checked, Qt::EditRole);

    } else {
        return QStyledItemDelegate::editorEvent(event, m, option, idx);
    }
}


void StitchLibraryDelegate::loadIcons(QComboBox *cb) const
{
    QStringList dirs, setDir;
    QString userFolder = Settings::inst()->userSettingsFolder();

    dirs << ":/stitches";

    QDir dir;
    dir.setPath(userFolder);

    //get all set folders.
    foreach(QString folder, dir.entryList(QDir::Dirs)) {
        if(folder != "." && folder != "..")
            dirs << userFolder + folder;
    }

    //get all files from all set folders.
    foreach(QString folder, dirs) {
        dir.setPath(folder);
        foreach(QString file, dir.entryList(QDir::Files)) {
            QIcon icon = QIcon(folder + "/" + file);
            QString name = QFileInfo(file).baseName();
            cb->addItem(icon, name, QVariant(folder + "/" + file));
        }
    }
}

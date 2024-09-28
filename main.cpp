#include <QTimer>
#include <QFileDialog>
#include <QCloseEvent>
#include <QDebug>
#include <QtEndian>
#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#include <QLoggingCategory>

#include "main.h"
#include "ui_main.h"

enum e_page
{
	e_page_main = 0,
	e_page_header,
	e_page_block,
};

main_t * main_t::ptr = NULL;
main_t::main_t(QWidget *parent) : QMainWindow(parent), m_ui(new Ui::main)
{
	m_ui->setupUi(this);

	main_t::ptr = this;

	connect(m_ui->le_part_number, SIGNAL(textChanged(const QString &)), this, SLOT(slt_header_changed()));

	m_ui->cb_part_type->addItem("EXE");
	m_ui->cb_part_type->addItem("SBL");
	m_ui->cb_part_type->addItem("DATA");
	m_ui->cb_part_type->addItem("CARCFG");
	m_ui->cb_part_type->addItem("CUSTOM");
	m_ui->cb_part_type->addItem("GBL");
	m_ui->cb_part_type->addItem("SIGCFG");
	m_ui->cb_part_type->addItem("TEST");
	connect(m_ui->cb_part_type, SIGNAL(currentIndexChanged(int)), this, SLOT(slt_header_changed()));

	m_ui->cb_network->addItem("CAN_MS");
	m_ui->cb_network->addItem("CAN_HS");
	m_ui->cb_network->addItem("{ CAN_MS, SUB_MOST }");
	m_ui->cb_network->addItem("FLEXRAY");
	m_ui->cb_network->addItem("DOIP");
	connect(m_ui->cb_network, SIGNAL(currentIndexChanged(int)), this, SLOT(slt_header_changed()));

	m_ui->cb_can_frame_format->addItem("STANDARD");
	m_ui->cb_can_frame_format->addItem("EXTENDED");
	m_ui->cb_can_frame_format->addItem("CAN_STANDARD");
	m_ui->cb_can_frame_format->addItem("CAN_EXTENDED");
	connect(m_ui->cb_can_frame_format, SIGNAL(currentIndexChanged(int)), this, SLOT(slt_header_changed()));

	connect(m_ui->sb_ecu_address, SIGNAL(valueChanged(int)), this, SLOT(slt_header_changed()));
	connect(m_ui->sb_call, SIGNAL(valueChanged(int)), this, SLOT(slt_header_changed()));
	connect(m_ui->cb_erase, SIGNAL(stateChanged(int)), this, SLOT(slt_header_changed()));

	m_ui->view->setModel(&list);
	m_ui->view->header()->resizeSections(QHeaderView::ResizeToContents);
	m_ui->view->setSelectionMode(QAbstractItemView::SingleSelection);
	m_ui->view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_ui->view->setAlternatingRowColors(true);
	m_ui->view->setRootIsDecorated(false);
	//m_ui->view->header()->setStretchLastSection(false);
	m_ui->view->setUniformRowHeights(true);
	connect(m_ui->view, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slt_view_clicked(const QModelIndex &)));
	connect(m_ui->view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(slt_selection_changed(const QItemSelection &)));

	connect(m_ui->btn_open, &QToolButton::clicked, this, &main_t::slt_btn_open);
	connect(m_ui->btn_save, &QToolButton::clicked, this, &main_t::slt_btn_save);
	connect(m_ui->btn_export, &QToolButton::clicked, this, &main_t::slt_btn_export);
	connect(m_ui->btn_import, &QToolButton::clicked, this, &main_t::slt_btn_import);
	connect(m_ui->btn_add, &QToolButton::clicked, this, &main_t::slt_btn_add);

	connect(m_ui->btn_block_open, &QToolButton::clicked, this, &main_t::slt_btn_block_open);
	connect(m_ui->btn_block_save, &QToolButton::clicked, this, &main_t::slt_btn_block_save);
	connect(m_ui->sb_block_addr, SIGNAL(valueChanged(double)), this, SLOT(slt_block_changed()));

	connect(m_ui->btn_about, &QToolButton::clicked, this, &main_t::slt_btn_about);

	m_ui->stack->setCurrentIndex(e_page_main);
}

main_t::~main_t()
{
	delete m_ui;
}

void main_t::slt_btn_about(int)
{
	QString os_type = QSysInfo::kernelType();
	QString os_version = QSysInfo::kernelVersion();
	QString version = QString("%1").arg(__DATE__);

	QMessageBox::about(this, tr("About qVBF"),
		tr("<h2>qVBF ") + version + "</h2>"
		"<p>qVBF is a simple editor of vbf files"
		"<p>Copyright: &copy; 2022 smartgauges@gmail.com"
		"<p>Source code: <a href=\"https://github.com/smartgauges/qvbf\">github.com/smartgauges/qvbf</a>"
		"<p>OS:" + os_type + " " + os_version + "");
}

void main_t::slt_btn_open()
{ 
	QString fileName;
	fileName = QFileDialog::getOpenFileName(this, tr("Open vbf file"), "./", tr("vbf (*.vbf *.VBF)"));

	vbf_t vbf;
	bool ret = vbf_open(fileName, vbf);
	if (!ret) {

		m_ui->statusBar->showMessage(tr("Open %1 failed").arg(fileName));
		return;
	}

	list.set(vbf);
	m_ui->stack->setCurrentIndex(e_page_main);

	load_header();

	QCoreApplication::processEvents();
	m_ui->view->header()->resizeSections(QHeaderView::ResizeToContents);
	m_ui->view->expandAll();

	setWindowTitle(fileName);

	m_ui->statusBar->showMessage(tr("Open %1").arg(fileName));
}

void main_t::slt_btn_save()
{
	QString fileName;
	fileName = QFileDialog::getSaveFileName(this, tr("Save vbf file"), QString(".") + QDir::separator() + QString("new.vbf"), tr("vbf (*.vbf *.VBF)"));

	if (fileName.isEmpty())
		return;

	const vbf_t & vbf = list.get();

	vbf_save(fileName, vbf);

	m_ui->statusBar->showMessage(tr("Saved %1").arg(fileName));
}

void main_t::slt_btn_add()
{
	QString fileName;
	fileName = QFileDialog::getOpenFileName(this, tr("Open bin file"), "./", tr("bin (*.bin *.BIN)"));

	bool ret = false;
	int idx = get_selected_row();
	if (idx <= 0)
		ret = list.add(fileName);
	else
		ret = list.insert(idx, fileName);

	if (!ret) {

		QString msg = (idx > 0) ? tr("Insert %1 failed") : tr("Add %1 failed");
		m_ui->statusBar->showMessage(msg.arg(fileName));
		return;
	}

	QCoreApplication::processEvents();
	m_ui->view->header()->resizeSections(QHeaderView::ResizeToContents);
	m_ui->view->expandAll();

	QString msg = (idx > 0) ? tr("Inserted %1") : tr("Added %1");
	m_ui->statusBar->showMessage(msg.arg(fileName));

	slt_header_changed();
}

void main_t::slt_btn_import()
{
	m_ui->statusBar->showMessage(tr("Import files"));

	vbf_t vbf = list.get();
	vbf_import(vbf);
	list.set(vbf);

	slt_header_changed();
}

void main_t::slt_btn_export()
{
	const vbf_t & vbf = list.get();
	vbf_export(vbf);
	m_ui->statusBar->showMessage(tr("Export files"));
}

void main_t::slt_view_clicked(const QModelIndex & idx)
{
	if (idx.column() != VbfModel::e_col_rm)
		return;

	m_ui->view->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

	m_ui->hexview->setData(NULL);
	list.rm(idx.row());

	QCoreApplication::processEvents();
	m_ui->view->header()->resizeSections(QHeaderView::ResizeToContents);
	m_ui->view->expandAll();

	slt_header_changed();
}

void main_t::slt_btn_block_open()
{
	int idx = get_selected_row();

	if (idx <= 0)
		return;

	QString fn = list.get().filename + "." + QString::number(idx - 1) + ".bin";

	QString fileName;
	fileName = QFileDialog::getOpenFileName(this, tr("Open bin file"), fn, tr("bin (*.bin *.BIN)"));

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return;

	QByteArray data = file.readAll();
	file.close();

	list.update_block(idx - 1, data);
	const block_t & block = list.get_block(idx - 1);
	m_ui->hexview->setData(&block.data);

	m_ui->statusBar->showMessage(tr("Update block %1 with %2 content").arg(idx).arg(fileName));

	slt_block_changed();
}

int main_t::get_selected_row()
{
	QModelIndexList sl = m_ui->view->selectionModel()->selectedIndexes();

	if (sl.isEmpty())
		return -1;

	if (sl.size() != VbfModel::e_col_nums)
		return -1;

	QModelIndex index = sl[VbfModel::e_col_block];

	return index.row();
}

void main_t::slt_btn_block_save()
{
	int idx = get_selected_row();
	if (idx <= 0)
		return;

	QString fn = list.get().filename + "." + QString::number(idx - 1) + ".bin";

	QString fileName;
	fileName = QFileDialog::getSaveFileName(this, tr("Save bin file"), fn, tr("bin (*.bin)"));

	if (fileName.isEmpty())
		return;

	const vbf_t & vbf = list.get();
	vbf_export_block(idx - 1, fileName, vbf);

	m_ui->statusBar->showMessage(tr("Saved block %1 to %2").arg(idx).arg(fileName));
}

void main_t::open_file_vbf(const QString & fileName)
{
	vbf_t vbf;
	if (vbf_open(fileName, vbf))
		list.set(vbf);

	load_header();

	QCoreApplication::processEvents();
	m_ui->view->header()->resizeSections(QHeaderView::ResizeToContents);
	m_ui->view->expandAll();
}

void main_t::load_header()
{
	const vbf_t & vbf = list.get();

	m_ui->le_part_number->blockSignals(true);
	m_ui->cb_part_type->blockSignals(true);
	m_ui->cb_network->blockSignals(true);
	m_ui->cb_can_frame_format->blockSignals(true);
	m_ui->sb_ecu_address->blockSignals(true);
	m_ui->sb_call->blockSignals(true);
	m_ui->cb_erase->blockSignals(true);

	m_ui->le_part_number->setText(vbf.header.sw_part_number);
	m_ui->cb_part_type->setCurrentText(vbf.header.sw_part_type);
	m_ui->cb_network->setCurrentText(vbf.header.network);
	m_ui->cb_can_frame_format->setCurrentText(vbf.header.can_frame_format);
	m_ui->sb_ecu_address->setValue(vbf.header.ecu_address);
	m_ui->sb_call->setValue(vbf.header.call);
	m_ui->sb_call->setEnabled((vbf.header.sw_part_type == "SBL") ? true : false);
	m_ui->cb_erase->setChecked(vbf.header.erases.size() ? true : false);

	m_ui->le_part_number->blockSignals(false);
	m_ui->cb_part_type->blockSignals(false);
	m_ui->cb_network->blockSignals(false);
	m_ui->cb_can_frame_format->blockSignals(false);
	m_ui->sb_ecu_address->blockSignals(false);
	m_ui->sb_call->blockSignals(false);
	m_ui->cb_erase->blockSignals(false);

	m_ui->text->clear();
	m_ui->text->insertPlainText(vbf.header.data);
}

void main_t::slt_selection_changed(const QItemSelection &)
{
	int idx = get_selected_row();

	//qDebug() << "select block row " << idx;

	if (idx < 0)
		return;

	//header
	if (!idx) {

		m_ui->stack->setCurrentIndex(e_page_header);

		load_header();

		m_ui->statusBar->showMessage(tr("Load header"));
	}
	else if (idx <= list.size()) {

		m_ui->sb_block_addr->blockSignals(true);
		const block_t & block = list.get_block(idx - 1);
		m_ui->sb_block_addr->setValue(block.addr);

		const vbf_t & vbf = list.get();
		QString slen;
		if (vbf.header.data_format_identifier_exist && vbf.header.data_format_identifier == 0x10)
			slen = QString("%1(%2)").arg(block.len).arg(block.data.size());
		else
			slen = QString("%1").arg(block.len);

		m_ui->lbl_block_size->setStyleSheet("QLabel {}");
		if (block.len > BLOCK_LIMIT_SIZE) {
			slen += " only first 1GB";
			m_ui->lbl_block_size->setStyleSheet("QLabel { background-color : red }");
		}
		m_ui->lbl_block_size->setText(slen);

		m_ui->hexview->setData(&block.data);
		m_ui->sb_block_addr->blockSignals(false);

		m_ui->statusBar->showMessage(tr("Load block %1").arg(idx));

		m_ui->stack->setCurrentIndex(e_page_block);
	}
}

void main_t::slt_header_changed()
{
	const vbf_t & vbf = list.get();

	header_t header = vbf.header;
	header.sw_part_number = m_ui->le_part_number->text();
	header.sw_part_type = m_ui->cb_part_type->currentText();
	header.network = m_ui->cb_network->currentText();
	header.can_frame_format = m_ui->cb_can_frame_format->currentText();
	header.ecu_address = m_ui->sb_ecu_address->value();
	header.call = m_ui->sb_call->value();

	if (!m_ui->cb_erase->isChecked())
		header.erases.clear();

	if (m_ui->cb_erase->isChecked() && !header.erases.size()) {

		for (int32_t i = 0; i < vbf.blocks.size(); i++) {

			const block_t & block = vbf.blocks[i];

			erase_t erase;
			erase.addr = block.addr;
			erase.size = block.len;
			header.erases.push_back(erase);
		}
	}

	m_ui->sb_call->setEnabled((header.sw_part_type == "SBL") ? true : false);

	list.update_header(header);
	const vbf_t & new_vbf = list.get();

	m_ui->text->clear();
	m_ui->text->insertPlainText(new_vbf.header.data);

	m_ui->statusBar->showMessage(tr("Update header"));
}

void main_t::slt_block_changed()
{
	int idx = get_selected_row();

	//qDebug() << "select block row " << idx;

	if (idx <= 0)
		return;

	if (idx <= list.size()) {

		list.update_block(idx - 1, m_ui->sb_block_addr->value());

		m_ui->statusBar->showMessage(tr("Update block %1 and header").arg(idx));

		slt_header_changed();
	}
}

void main_t::slt_log(uint8_t lvl, const QString & txt)
{
	QDateTime dt = QDateTime::currentDateTime();
	QString st = dt.toString("hh:mm:ss.z");
	if (lvl == e_log_warn)
		m_ui->log->appendHtml(st + " " + "<font color = \"red\">" + txt + "</font>");
	else if (lvl == e_log_debug)
		m_ui->log->appendHtml(st + " " + "<font color = \"blue\">" + txt + "</font>");
	else
		m_ui->log->appendPlainText(st + " " + txt);
}

void main_t::QDebugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	QString message = qFormatLogMessage(type, context, msg);

	e_log_level lvl = e_log_info;

	switch(type) {
		case QtMsgType::QtDebugMsg:
			lvl = e_log_debug;
			break;
		case QtMsgType::QtWarningMsg:
			lvl = e_log_warn;
			break;
		default:
			lvl = e_log_info;
			break;
	}

	if (main_t::ptr) {
		main_t::ptr->main_t::slt_log(lvl, qPrintable(message));
		QCoreApplication::processEvents();
	}
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QLoggingCategory::setFilterRules("*.debug=true\n""qt.*.debug=false");

	main_t w;
	qInstallMessageHandler(main_t::QDebugMessageHandler);
	w.show();

	if (argc == 2) {

		w.open_file_vbf(argv[1]);
	}

	return a.exec();
}


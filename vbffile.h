#ifndef VBFFILE_H
#define VBFFILE_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <inttypes.h>

struct block_t
{
	uint32_t addr;
	uint32_t len;
	QByteArray data;
	QByteArray cdata;
	uint16_t crc;
	uint8_t percent;

	block_t()
	{
		reset();
	}

	void reset()
	{
		data.clear();
		len = 0;
		percent = 0;
	}
};

struct erase_t
{
	uint32_t addr;
	uint32_t size;
};

struct header_t
{
	QString version;
	QByteArray data;
	QString sw_part_number;
	QString sw_part_type;
	QString network;
	QString can_frame_format;
	uint32_t ecu_address;
	uint32_t call;
	QVector <erase_t> erases;
	uint32_t file_checksum;
	uint32_t file_checksum_offset;
	bool data_format_identifier_exist;
	uint32_t data_format_identifier;

	header_t()
	{
		reset();
	}

	void reset()
	{
		data.clear();
		sw_part_number = QString();
		sw_part_type = QString();
		network = QString();
		can_frame_format = QString();
		ecu_address = 0x0;
		call = 0x0;
		erases.resize(0);
		file_checksum = 0x0;
		data_format_identifier_exist = false;
		data_format_identifier = 0;
	}
};

struct vbf_t
{
	QString filename;
	header_t header;
	QVector <block_t> blocks;
	uint32_t size;
	uint8_t percent;
	uint8_t block_idx;

	vbf_t()
	{
		reset();
	}

	void reset()
	{
		filename = QString();
		header.reset();
		blocks.resize(0);
		size = 0;
		percent = 0;
		block_idx = 0;
	}
};

bool vbf_add(const QString & fileName, vbf_t & vbf);

bool vbf_insert(int idx, const QString & fileName, vbf_t & vbf);

bool vbf_open(const QString & fileName, vbf_t & vbf);

void vbf_export(const vbf_t & vbf);

void vbf_import(vbf_t & vbf);

void vbf_save(const QString & fileName, const vbf_t & vbf);

void vbf_update_header(vbf_t & vbf);

#endif


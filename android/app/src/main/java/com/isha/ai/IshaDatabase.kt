package com.isha.ai

import android.content.ContentValues
import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper

class IshaDatabase(context: Context) : SQLiteOpenHelper(context, DATABASE_NAME, null, DATABASE_VERSION) {

    companion object {
        const val DATABASE_NAME = "isha_rag.db"
        const val DATABASE_VERSION = 1

        const val TABLE_DOCUMENTS = "documents"
        const val COL_DOC_ID = "id"
        const val COL_DOC_PATH = "path"
        const val COL_DOC_HASH = "hash"
        const val COL_DOC_SIZE = "file_size"
        const val COL_DOC_CHUNKS = "chunk_count"
        const val COL_DOC_TIME = "last_indexed"

        const val TABLE_CHUNKS = "chunks"
        const val COL_CHUNK_ID = "id"
        const val COL_CHUNK_DOC_ID = "document_id"
        const val COL_CHUNK_PAGE = "page_num"
        const val COL_CHUNK_SEQ = "seq_index"
        const val COL_CHUNK_HASH = "chunk_hash"
        const val COL_CHUNK_TEXT = "text"
    }

    override fun onCreate(db: SQLiteDatabase) {
        val createDocTable = """
            CREATE TABLE $TABLE_DOCUMENTS (
                $COL_DOC_ID INTEGER PRIMARY KEY AUTOINCREMENT,
                $COL_DOC_PATH TEXT UNIQUE,
                $COL_DOC_HASH TEXT,
                $COL_DOC_SIZE INTEGER,
                $COL_DOC_CHUNKS INTEGER,
                $COL_DOC_TIME INTEGER
            )
        """.trimIndent()

        val createChunkTable = """
            CREATE TABLE $TABLE_CHUNKS (
                $COL_CHUNK_ID INTEGER PRIMARY KEY AUTOINCREMENT,
                $COL_CHUNK_DOC_ID INTEGER,
                $COL_CHUNK_PAGE INTEGER,
                $COL_CHUNK_SEQ INTEGER,
                $COL_CHUNK_HASH TEXT,
                $COL_CHUNK_TEXT TEXT,
                FOREIGN KEY($COL_CHUNK_DOC_ID) REFERENCES $TABLE_DOCUMENTS($COL_DOC_ID) ON DELETE CASCADE
            )
        """.trimIndent()

        db.execSQL(createDocTable)
        db.execSQL(createChunkTable)
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        db.execSQL("DROP TABLE IF EXISTS $TABLE_CHUNKS")
        db.execSQL("DROP TABLE IF EXISTS $TABLE_DOCUMENTS")
        onCreate(db)
    }

    fun registerDocument(path: String, hash: String, fileSize: Long): Long {
        val db = writableDatabase
        val values = ContentValues().apply {
            put(COL_DOC_PATH, path)
            put(COL_DOC_HASH, hash)
            put(COL_DOC_SIZE, fileSize)
            put(COL_DOC_CHUNKS, 0)
            put(COL_DOC_TIME, System.currentTimeMillis())
        }
        return db.insertWithOnConflict(TABLE_DOCUMENTS, null, values, SQLiteDatabase.CONFLICT_REPLACE)
    }

    fun updateDocChunkCount(docId: Long, chunkCount: Int) {
        val db = writableDatabase
        val values = ContentValues().apply {
            put(COL_DOC_CHUNKS, chunkCount)
        }
        db.update(TABLE_DOCUMENTS, values, "$COL_DOC_ID = ?", arrayOf(docId.toString()))
    }

    fun getDocumentId(path: String): Long {
        val db = readableDatabase
        val cursor = db.query(TABLE_DOCUMENTS, arrayOf(COL_DOC_ID), "$COL_DOC_PATH = ?", arrayOf(path), null, null, null)
        val id = if (cursor.moveToFirst()) cursor.getLong(0) else -1L
        cursor.close()
        return id
    }

    fun getDocumentHash(path: String): String? {
        val db = readableDatabase
        val cursor = db.query(TABLE_DOCUMENTS, arrayOf(COL_DOC_HASH), "$COL_DOC_PATH = ?", arrayOf(path), null, null, null)
        val hash = if (cursor.moveToFirst()) cursor.getString(0) else null
        cursor.close()
        return hash
    }

    fun insertChunksBatched(docId: Long, chunks: List<Pair<Int, String>>, chunkHashes: List<String>) {
        val db = writableDatabase
        db.beginTransaction()
        try {
            // Delete old chunks first
            db.delete(TABLE_CHUNKS, "$COL_CHUNK_DOC_ID = ?", arrayOf(docId.toString()))

            for (i in chunks.indices) {
                val values = ContentValues().apply {
                    put(COL_CHUNK_DOC_ID, docId)
                    put(COL_CHUNK_PAGE, chunks[i].first)
                    put(COL_CHUNK_SEQ, i)
                    put(COL_CHUNK_HASH, chunkHashes[i])
                    put(COL_CHUNK_TEXT, chunks[i].second)
                }
                db.insert(TABLE_CHUNKS, null, values)
            }
            db.setTransactionSuccessful()
        } finally {
            db.endTransaction()
        }
    }

    fun deleteDocument(path: String) {
        val db = writableDatabase
        db.beginTransaction()
        try {
            val docId = getDocumentId(path)
            if (docId != -1L) {
                db.delete(TABLE_CHUNKS, "$COL_CHUNK_DOC_ID = ?", arrayOf(docId.toString()))
                db.delete(TABLE_DOCUMENTS, "$COL_DOC_ID = ?", arrayOf(docId.toString()))
            }
            db.setTransactionSuccessful()
        } finally {
            db.endTransaction()
        }
    }

    fun getDocumentsList(): List<DocumentInfo> {
        val list = mutableListOf<DocumentInfo>()
        val db = readableDatabase
        val cursor = db.query(TABLE_DOCUMENTS, null, null, null, null, null, "$COL_DOC_TIME DESC")
        if (cursor.moveToFirst()) {
            do {
                val id = cursor.getLong(cursor.getColumnIndexOrThrow(COL_DOC_ID))
                val path = cursor.getString(cursor.getColumnIndexOrThrow(COL_DOC_PATH))
                val hash = cursor.getString(cursor.getColumnIndexOrThrow(COL_DOC_HASH))
                val size = cursor.getLong(cursor.getColumnIndexOrThrow(COL_DOC_SIZE))
                val chunks = cursor.getInt(cursor.getColumnIndexOrThrow(COL_DOC_CHUNKS))
                val time = cursor.getLong(cursor.getColumnIndexOrThrow(COL_DOC_TIME))
                list.add(DocumentInfo(id, path, hash, size, chunks, time))
            } while (cursor.moveToNext())
        }
        cursor.close()
        return list
    }
}

val Context.db: IshaDatabase
    get() = IshaDatabase(this)

data class DocumentInfo(
    val id: Long,
    val path: String,
    val hash: String,
    val fileSize: Long,
    val chunkCount: Int,
    val lastIndexed: Long
)

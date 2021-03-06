/*
 * sqliteinterface_SqliteBackend.cpp.cpp
 */

#include <stddef.h>
#include <string>
#include <sqlite3.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "utilities/mutgos_config.h"
#include "text/text_StringConversion.h"

#include "sqliteinterface_SqliteBackend.h"

#include "concurrency/concurrency_ReaderLockToken.h"

#include "utilities/utility_MemoryBuffer.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityType.h"
#include "concurrency/concurrency_WriterLockToken.h"

#include "logging/log_Logger.h"


namespace mutgos
{
namespace sqliteinterface
{
    // ----------------------------------------------------------------------
    SqliteBackend::SqliteBackend(void)
      : dbhandle_ptr(0),
        list_sites_stmt(0),
        list_deleted_sites_stmt(0),
        list_all_entities_site_stmt(0),
        find_name_in_db_stmt(0),
        find_name_type_in_db_stmt(0),
        find_exact_name_type_in_db_stmt(0),
        get_entity_type_stmt(0),
        undelete_site_stmt(0),
        next_site_id_stmt(0),
        insert_first_next_site_id_stmt(0),
        update_next_site_id_stmt(0),
        insert_new_site_stmt(0),
        insert_first_site_entity_id_stmt(0),
        update_entity_stmt(0),
        get_entity_stmt(0),
        delete_site_entities_stmt(0),
        delete_site_display_names_stmt(0),
        get_next_deleted_entity_id_stmt(0),
        mark_deleted_id_used_stmt(0),
        get_next_entity_id_stmt(0),
        update_next_entity_id_stmt(0),
        add_entity_stmt(0),
        delete_entity_stmt(0),
        add_reuse_entity_id_stmt(0),
        mark_site_deleted_stmt(0),
        delete_all_site_entity_id_reuse_stmt(0),
        delete_site_next_entity_id_stmt(0)
    {
    }

    // ----------------------------------------------------------------------
    SqliteBackend::~SqliteBackend()
    {
        shutdown();
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::init(void)
    {
        LOG(info, "sqliteinterface", "init", "Starting up...");

        bool success = true;

        if (not dbhandle_ptr)
        {
            LOG(info, "sqliteinterface", "init", "Mounting database...");

            const int rc = sqlite3_open(
                config::db::db_file().c_str(),
                &dbhandle_ptr);
            success = (rc == SQLITE_OK);

            if (success)
            {
                success = (sqlite3_exec(
                    dbhandle_ptr,
                    "PRAGMA main.PAGE_SIZE=8192;",
                    0,
                    0,
                    0) == SQLITE_OK) and
                    (sqlite3_exec(
                        dbhandle_ptr,
                        "PRAGMA journal_mode=WAL;",
                        0,
                        0,
                        0) == SQLITE_OK) and
                    (sqlite3_exec(
                        dbhandle_ptr,
                        "PRAGMA synchronous=NORMAL;",
                        0,
                        0,
                        0) == SQLITE_OK)
                    and create_tables() and sql_init();

                if (success)
                {
                    LOG(info, "sqliteinterface", "init", "Database mounted.");
                }
                else
                {
                    LOG(fatal, "sqliteinterface", "init",
                        "Unable to configure SQL.");
                }
            }
            else
            {
                LOG(fatal, "sqliteinterface", "init",
                    "Unable to mount: " + std::string(sqlite3_errstr(rc)));
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::shutdown(void)
    {
        LOG(info, "sqliteinterface", "shutdown", "Shutting down...");

        bool success = not any_mem_owned();

        if (success and dbhandle_ptr)
        {
            sqlite3_finalize(list_sites_stmt);
            list_sites_stmt = 0;

            sqlite3_finalize(list_deleted_sites_stmt);
            list_deleted_sites_stmt = 0;

            sqlite3_finalize(list_all_entities_site_stmt);
            list_all_entities_site_stmt = 0;

            sqlite3_finalize(find_name_in_db_stmt);
            find_name_in_db_stmt = 0;

            sqlite3_finalize(find_name_type_in_db_stmt);
            find_name_type_in_db_stmt = 0;

            sqlite3_finalize(find_exact_name_type_in_db_stmt);
            find_exact_name_type_in_db_stmt = 0;

            sqlite3_finalize(get_entity_type_stmt);
            get_entity_type_stmt = 0;

            sqlite3_finalize(undelete_site_stmt);
            undelete_site_stmt = 0;

            sqlite3_finalize(next_site_id_stmt);
            next_site_id_stmt = 0;

            sqlite3_finalize(insert_first_next_site_id_stmt);
            insert_first_next_site_id_stmt = 0;

            sqlite3_finalize(update_next_site_id_stmt);
            update_next_site_id_stmt = 0;

            sqlite3_finalize(insert_new_site_stmt);
            insert_new_site_stmt = 0;

            sqlite3_finalize(insert_first_site_entity_id_stmt);
            insert_first_site_entity_id_stmt = 0;

            sqlite3_finalize(update_entity_stmt);
            update_entity_stmt = 0;

            sqlite3_finalize(get_entity_stmt);
            get_entity_stmt = 0;

            sqlite3_finalize(delete_site_entities_stmt);
            delete_site_entities_stmt = 0;

            sqlite3_finalize(delete_site_display_names_stmt);
            delete_site_display_names_stmt = 0;

            sqlite3_finalize(get_next_deleted_entity_id_stmt);
            get_next_deleted_entity_id_stmt = 0;

            sqlite3_finalize(mark_deleted_id_used_stmt);
            mark_deleted_id_used_stmt = 0;

            sqlite3_finalize(get_next_entity_id_stmt);
            get_next_entity_id_stmt = 0;

            sqlite3_finalize(update_next_entity_id_stmt);
            update_next_entity_id_stmt = 0;

            sqlite3_finalize(add_entity_stmt);
            add_entity_stmt = 0;

            sqlite3_finalize(delete_entity_stmt);
            delete_entity_stmt = 0;

            sqlite3_finalize(add_reuse_entity_id_stmt);
            add_reuse_entity_id_stmt = 0;

            sqlite3_finalize(mark_site_deleted_stmt);
            mark_site_deleted_stmt = 0;

            sqlite3_finalize(delete_all_site_entity_id_reuse_stmt);
            delete_all_site_entity_id_reuse_stmt = 0;

            sqlite3_finalize(delete_site_next_entity_id_stmt);
            delete_site_next_entity_id_stmt = 0;

            success = (sqlite3_close(dbhandle_ptr) == SQLITE_OK);

            if (success)
            {
                dbhandle_ptr = 0;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    std::string SqliteBackend::get_backend_name(void)
    {
        return "SQLite3";
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::entity_mem_owned_by_this(
        const dbtype::Entity *entity_ptr)
    {
        return is_mem_owned(entity_ptr);
    }

    // ----------------------------------------------------------------------
    void SqliteBackend::delete_entity_mem(dbtype::Entity *entity_ptr)
    {
        if (removed_mem_owned(entity_ptr))
        {
            delete entity_ptr;
        }
    }

    // ----------------------------------------------------------------------
    dbtype::Entity *SqliteBackend::new_entity(
        const dbtype::EntityType type,
        const dbtype::Id::SiteIdType site_id,
        const dbtype::Id &owner,
        const std::string &name)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        dbtype::Entity *entity_ptr = 0;
        dbtype::Id::EntityIdType entity_id = 0;
        int rc = SQLITE_OK;
        bool fatal_error = false;

        // See if there is a deleted ID we can reuse.
        // If so, remove from deleted list
        //
        if (sqlite3_bind_int(
            get_next_deleted_entity_id_stmt,
            sqlite3_bind_parameter_index(get_next_deleted_entity_id_stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "new_entity",
                "For get_next_deleted_entity_id_stmt, could not bind $SITEID");
        }

        // Only stepping once since we just need one.
        rc = sqlite3_step(get_next_deleted_entity_id_stmt);

        if (rc == SQLITE_ROW )
        {
            // There is an ID we can use!
            //
            entity_id = sqlite3_column_int64(get_next_deleted_entity_id_stmt, 0);

            fatal_error = not entity_id;

            // Remove ID from the reuse table, since we are now using it.
            //
            if (sqlite3_bind_int(
                mark_deleted_id_used_stmt,
                sqlite3_bind_parameter_index(mark_deleted_id_used_stmt, "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "new_entity",
                    "For mark_deleted_id_used_stmt, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                mark_deleted_id_used_stmt,
                sqlite3_bind_parameter_index(mark_deleted_id_used_stmt, "$ENTITYID"),
                entity_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "new_entity",
                    "For mark_deleted_id_used_stmt, could not bind $ENTITYID");
            }

            rc = sqlite3_step(mark_deleted_id_used_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "new_entity",
                    "Could not remove selected ID from reuse list: "
                    + std::string(sqlite3_errstr(rc)));

                fatal_error = true;
            }
        }
        else if (rc != SQLITE_DONE)
        {
            // An error occurred.
            //
            LOG(error, "sqliteinterface", "new_entity",
                "Could not get next available recycled ID: "
                + std::string(sqlite3_errstr(rc)));

            fatal_error = true;
        }

        reset(get_next_deleted_entity_id_stmt);
        reset(mark_deleted_id_used_stmt);

        // If no existing ID, get a fresh one
        //
        if (not fatal_error and (not entity_id))
        {
            if (sqlite3_bind_int(
                get_next_entity_id_stmt,
                sqlite3_bind_parameter_index(get_next_entity_id_stmt, "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "new_entity",
                    "For get_next_entity_id_stmt, could not bind $SITEID");
            }

            rc = sqlite3_step(get_next_entity_id_stmt);

            if (rc != SQLITE_ROW)
            {
                LOG(error, "sqliteinterface", "new_entity",
                    "Could not get next fresh ID: "
                    + std::string(sqlite3_errstr(rc)));

                fatal_error = true;
            }
            else
            {
                entity_id =
                    (dbtype::Id::EntityIdType) sqlite3_column_int64(
                        get_next_entity_id_stmt, 0);

                fatal_error = not entity_id;
            }

            if (not fatal_error)
            {
                if (sqlite3_bind_int(
                    update_next_entity_id_stmt,
                    sqlite3_bind_parameter_index(update_next_entity_id_stmt, "$SITEID"),
                    site_id) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_entity",
                        "For update_next_entity_id_stmt, could not bind $SITEID");
                }

                if (sqlite3_bind_int64(
                    update_next_entity_id_stmt,
                    sqlite3_bind_parameter_index(update_next_entity_id_stmt, "$NEXTID"),
                    entity_id + 1) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_entity",
                        "For update_next_entity_id_stmt, could not bind $NEXTID");
                }

                rc = sqlite3_step(update_next_entity_id_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_entity",
                        "Could not update next fresh ID: "
                        + std::string(sqlite3_errstr(rc)));

                    fatal_error = true;
                }
            }
        }

        reset(get_next_entity_id_stmt);
        reset(update_next_entity_id_stmt);

        entity_ptr = fatal_error ? 0 : make_new_entity(
            type,
            dbtype::Id(site_id, entity_id),
            owner,
            name);
        fatal_error = not entity_ptr;

        if (entity_ptr)
        {
            concurrency::WriterLockToken token(*entity_ptr);

            fatal_error = not bind_entity_update_params(
                entity_ptr,
                token,
                add_entity_stmt);

            if (sqlite3_bind_int(
                add_entity_stmt,
                sqlite3_bind_parameter_index(add_entity_stmt, "$VERSION"),
                entity_ptr->get_entity_version()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "new_entity",
                    "For add_entity_stmt, could not bind $VERSION");
            }

            if (fatal_error)
            {
                // Could not complete binding, abort
                //
                LOG(error, "sqliteinterface", "new_entity",
                    "Binding did not complete.  Aborted.  SiteID: "
                      + text::to_string(site_id)
                      + "  EntityID: "
                      + text::to_string(entity_id));
            }
            else
            {
                // Commit and return entity
                //
                const int rc = sqlite3_step(add_entity_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "save_entity_db",
                        "Could not save new Entity: "
                        + std::string(sqlite3_errstr(rc)));
                    fatal_error = true;
                }
            }
        }

        if (fatal_error)
        {
            delete entity_ptr;
            entity_ptr = 0;
        }

        reset(add_entity_stmt);

        return entity_ptr;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity *SqliteBackend::get_entity_db(const dbtype::Id &id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        dbtype::Entity *entity_ptr = get_entity_pointer(id);

        if (not entity_ptr)
        {
            // Not in memory, try and get it from the database
            //
            if (sqlite3_bind_int(
                get_entity_stmt,
                sqlite3_bind_parameter_index(get_entity_stmt, "$SITEID"),
                id.get_site_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "get_entity_db",
                    "For get_entity_stmt, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                get_entity_stmt,
                sqlite3_bind_parameter_index(get_entity_stmt, "$ENTITYID"),
                id.get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "get_entity_db",
                    "For get_entity_stmt, could not bind $ENTITYID");
            }

            // Should be 0 or 1 lines
            //
            if (sqlite3_step(get_entity_stmt) == SQLITE_ROW)
            {
                // Entity exists.  Deserialize it.
                //
                const int entity_type_int = sqlite3_column_int(get_entity_stmt, 0);
                const void *blob_ptr = sqlite3_column_blob(get_entity_stmt, 1);
                const int blob_size = sqlite3_column_bytes(get_entity_stmt, 1);

                if ((not blob_ptr) or (blob_size <= 0))
                {
                    LOG(error, "sqliteinterface", "get_entity_db",
                        "No blob data for ID " + id.to_string(true));
                }
                else
                {
                    const dbtype::EntityType entity_type =
                        (dbtype::EntityType) entity_type_int;

                    utility::MemoryBuffer buffer(blob_ptr, blob_size);
                    entity_ptr = make_deserialize_entity(entity_type, buffer);

                    if (not entity_ptr)
                    {
                        LOG(error, "sqliteinterface", "get_entity_db",
                            "Unknown type to deserialize: "
                              + dbtype::entity_type_to_string(entity_type)
                              + "  ID: " + id.to_string(true));
                    }
                    else
                    {
                        // Put into lookup map
                        added_mem_owned(entity_ptr);
                    }
                }
            }

            reset(get_entity_stmt);
        }

        return entity_ptr;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::save_entity_db(dbtype::Entity *entity_ptr)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        bool success = entity_ptr and is_mem_owned(entity_ptr);

        if (success)
        {
            concurrency::WriterLockToken token(*entity_ptr);

            success = bind_entity_update_params(
                entity_ptr,
                token,
                update_entity_stmt);

            if (not success)
            {
                LOG(error, "sqliteinterface", "save_entity_db",
                    "Could not save entity!");
            }

            if (success)
            {
                const int rc = sqlite3_step(update_entity_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "save_entity_db",
                        "Could not update Entity: "
                        + std::string(sqlite3_errstr(rc)));
                    success = false;
                }
                else
                {
                    entity_ptr->clear_dirty(token);
                }
            }

            reset(update_entity_stmt);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::delete_entity_db(const dbtype::Id &id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        // Confirm not still in memory
        const bool success = not is_mem_owned(id);

        if (success and (not id.is_default()))
        {
            bool delete_good = true;
            int rc = SQLITE_OK;

            // Delete from entities table
            //

            if (sqlite3_bind_int(
                delete_entity_stmt,
                sqlite3_bind_parameter_index(delete_entity_stmt, "$SITEID"),
                id.get_site_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_entity_db",
                    "For delete_entity_stmt, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                delete_entity_stmt,
                sqlite3_bind_parameter_index(delete_entity_stmt, "$ENTITYID"),
                id.get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_entity_db",
                    "For delete_entity_stmt, could not bind $ENTITYID");
            }

            rc = sqlite3_step(delete_entity_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "delete_entity_db",
                    "Could not delete Entity: "
                    + std::string(sqlite3_errstr(rc)));
                delete_good = false;
            }

            reset(delete_entity_stmt);

            if (delete_good)
            {
                // Delete worked, add ID into table for future reuse
                //
                if (sqlite3_bind_int(
                    add_reuse_entity_id_stmt,
                    sqlite3_bind_parameter_index(add_reuse_entity_id_stmt, "$SITEID"),
                    id.get_site_id()) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "delete_entity_db",
                        "For add_reuse_entity_id_stmt, could not bind $SITEID");
                }

                if (sqlite3_bind_int64(
                    add_reuse_entity_id_stmt,
                    sqlite3_bind_parameter_index(add_reuse_entity_id_stmt, "$ENTITYID"),
                    id.get_entity_id()) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "delete_entity_db",
                        "For add_reuse_entity_id_stmt, could not bind $ENTITYID");
                }

                rc = sqlite3_step(add_reuse_entity_id_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "delete_entity_db",
                        "Could not insert Entity ID into reuse table: "
                        + std::string(sqlite3_errstr(rc)));
                }

                reset(add_reuse_entity_id_stmt);
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    dbtype::EntityType SqliteBackend::get_entity_type_db(const dbtype::Id &id)
    {
        dbtype::EntityType entity_type = dbtype::ENTITYTYPE_invalid;

        boost::lock_guard<boost::mutex> guard(mutex);
        dbtype::Entity * const entity_ptr = get_entity_pointer(id);

        if (entity_ptr)
        {
            // In our cache of Entities in use.  Return the type.
            entity_type = entity_ptr->get_entity_type();
        }
        else
        {
            // Not in cache, try and get it from the database
            //
            if (sqlite3_bind_int(
                get_entity_type_stmt,
                sqlite3_bind_parameter_index(get_entity_stmt, "$SITEID"),
                id.get_site_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "get_entity_type_db",
                    "For get_entity_type_stmt, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                get_entity_type_stmt,
                sqlite3_bind_parameter_index(get_entity_stmt, "$ENTITYID"),
                id.get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "get_entity_type_db",
                    "For get_entity_type_stmt, could not bind $ENTITYID");
            }

            // Should be 0 or 1 lines
            //
            if (sqlite3_step(get_entity_type_stmt) == SQLITE_ROW)
            {
                // Entity exists.  Get the type.
                //
                const int entity_type_int =
                    sqlite3_column_int(get_entity_type_stmt, 0);

                entity_type = (dbtype::EntityType) entity_type_int;
            }

            reset(get_entity_type_stmt);
        }

        return entity_type;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector SqliteBackend::find_in_db(
        const dbtype::Id::SiteIdType site_id,
        const dbtype::EntityType type,
        const std::string &name,
        const bool exact)
    {
        dbtype::Entity::IdVector result;
        boost::lock_guard<boost::mutex> guard(mutex);

        sqlite3_stmt *stmt = find_name_type_in_db_stmt;

        if (exact)
        {
            stmt = find_exact_name_type_in_db_stmt;
        }

        if (sqlite3_bind_int(
            stmt,
            sqlite3_bind_parameter_index(stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_in_db(type, name)",
                "For stmt, could not bind $SITEID");
        }

        if (sqlite3_bind_int(
            stmt,
            sqlite3_bind_parameter_index(stmt, "$TYPE"),
            type) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_in_db(type, name)",
                "For stmt, could not bind $TYPE");
        }

        if (sqlite3_bind_text(
            stmt,
            sqlite3_bind_parameter_index(stmt, "$NAME"),
            name.c_str(),
            name.size(),
            SQLITE_TRANSIENT) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_in_db(type, name)",
                "For stmt, could not bind $NAME");
        }

        add_entity_ids(stmt, site_id, result);

        reset(stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector SqliteBackend::find_in_db(
        const dbtype::Id::SiteIdType site_id,
        const std::string &name)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        dbtype::Entity::IdVector result;

        if (sqlite3_bind_int(
            find_name_in_db_stmt,
            sqlite3_bind_parameter_index(find_name_in_db_stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_in_db(name)",
                "For find_name_in_db_stmt, could not bind $SITEID");
        }

        if (sqlite3_bind_text(
            find_name_in_db_stmt,
            sqlite3_bind_parameter_index(find_name_in_db_stmt, "$NAME"),
            name.c_str(),
            name.size(),
            SQLITE_TRANSIENT) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_in_db(name)",
                "For find_name_in_db_stmt, could not bind $NAME");
        }

        add_entity_ids(find_name_in_db_stmt, site_id, result);

        reset(find_name_in_db_stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector SqliteBackend::find_in_db(
        const dbtype::Id::SiteIdType site_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        dbtype::Entity::IdVector result;

        if (sqlite3_bind_int(
            list_all_entities_site_stmt,
            sqlite3_bind_parameter_index(list_all_entities_site_stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_in_db(site)",
                "For list_all_entities_site_stmt, could not bind $SITEID");
        }

        add_entity_ids(list_all_entities_site_stmt, site_id, result);

        reset(list_all_entities_site_stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Id::SiteIdVector SqliteBackend::get_site_ids_in_db(void)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        dbtype::Id::SiteIdVector result;
        int rc = sqlite3_step(list_sites_stmt);

        while (rc == SQLITE_ROW)
        {
            result.push_back(
                (dbtype::Id::SiteIdType) sqlite3_column_int(list_sites_stmt, 0));
            rc = sqlite3_step(list_sites_stmt);
        }

        if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "get_site_ids_in_db",
                "Error getting list of site IDs: "
                + std::string(sqlite3_errstr(rc)));
        }

        reset(list_sites_stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::new_site_in_db(dbtype::Id::SiteIdType &site_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        bool success = false;
        int rc = SQLITE_OK;

        // Query all deleted sites.  Pick first.  Mark as not deleted. Done.

        // See if we can reuse a site that has been deleted.
        //
        rc = sqlite3_step(list_deleted_sites_stmt);

        if (rc == SQLITE_ROW)
        {
            // Found one we can reuse!
            //
            site_id = (dbtype::Id::SiteIdType)
                sqlite3_column_int(list_deleted_sites_stmt, 0);

            // Update site as no longer deleted.
            //
            if (sqlite3_bind_int(
                undelete_site_stmt,
                sqlite3_bind_parameter_index(undelete_site_stmt, "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "new_site_in_db",
                    "For undelete_site_stmt, could not bind $SITEID");
            }

            rc = sqlite3_step(undelete_site_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "new_site_in_db",
                    "Could not reuse site: "
                    + std::string(sqlite3_errstr(rc)));
            }
            else
            {
                rc = SQLITE_OK;
                success = true;
            }

            reset(undelete_site_stmt);
        }
        else if (rc != SQLITE_DONE)
        {
            // Query errored out
            LOG(error, "sqliteinterface", "new_site_in_db",
                "Error getting list of deleted site IDs: "
                + std::string(sqlite3_errstr(rc)));
        }
        else
        {
            // So the next section knows it wasn't an error that stopped us
            // from getting an ID.
            rc = SQLITE_OK;
        }

        reset(list_deleted_sites_stmt);

        if ((not success) and (rc == SQLITE_OK))
        {
            // No available deleted sites to reuse.  Get a new one.
            //
            rc = sqlite3_step(next_site_id_stmt);

            if (rc == SQLITE_ROW)
            {
                // Found next ID.  Use it, and increment in table.
                //
                site_id = (dbtype::Id::SiteIdType)
                    sqlite3_column_int(next_site_id_stmt, 0);

                if (sqlite3_bind_int(
                    update_next_site_id_stmt,
                    sqlite3_bind_parameter_index(
                        update_next_site_id_stmt,
                        "$SITEID"),
                    site_id + 1) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "For update_next_site_id_stmt, could not bind $SITEID");
                }

                rc = sqlite3_step(update_next_site_id_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "Could not update next site ID: "
                        + std::string(sqlite3_errstr(rc)));
                }
                else
                {
                    rc = SQLITE_OK;
                    success = true;
                }

                reset(update_next_site_id_stmt);
            }
            else
            {
                // No next ID found - first use.  Insert default and
                // use it.
                //
                rc = sqlite3_step(insert_first_next_site_id_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "Could not insert first next site ID: "
                        + std::string(sqlite3_errstr(rc)));
                }
                else
                {
                    rc = SQLITE_OK;
                    success = true;
                    site_id = 1;
                }

                reset(insert_first_next_site_id_stmt);
            }

            reset(next_site_id_stmt);

            if (success)
            {
                // Insert the new site into the sites table
                //

                if (sqlite3_bind_int(
                    insert_new_site_stmt,
                    sqlite3_bind_parameter_index(
                        insert_new_site_stmt,
                        "$SITEID"),
                    site_id) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "For insert_new_site_stmt, could not bind $SITEID");
                }

                rc = sqlite3_step(insert_new_site_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "Could not insert new site: "
                        + std::string(sqlite3_errstr(rc)));

                    success = false;
                }
                else
                {
                    rc = SQLITE_OK;
                }

                reset(insert_new_site_stmt);

                // Delete all existing entity data with that site ID in case
                // anything is still there.
                //
                delete_site_entity_data(site_id);
            }

            if (success)
            {
                if (sqlite3_bind_int(
                    insert_first_site_entity_id_stmt,
                    sqlite3_bind_parameter_index(
                        insert_first_site_entity_id_stmt,
                        "$SITEID"),
                    site_id) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "For insert_first_site_entity_id_stmt, could not bind $SITEID");
                }

                rc = sqlite3_step(insert_first_site_entity_id_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "Could not insert new site first entity ID: "
                        + std::string(sqlite3_errstr(rc)));

                    success = false;
                }
                else
                {
                    rc = SQLITE_OK;
                }

                reset(insert_first_site_entity_id_stmt);
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::delete_site_in_db(const dbtype::Id::SiteIdType site_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        bool success = delete_site_entity_data(site_id);
        int rc = SQLITE_OK;

        if (success)
        {
            if (sqlite3_bind_int(
                mark_site_deleted_stmt,
                sqlite3_bind_parameter_index(
                    mark_site_deleted_stmt,
                    "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "For mark_site_deleted_stmt, could not bind $SITEID");
            }


            rc = sqlite3_step(mark_site_deleted_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "Could not mark site as deleted: "
                    + std::string(sqlite3_errstr(rc)));

                success = false;
            }

            if (sqlite3_bind_int(
                delete_all_site_entity_id_reuse_stmt,
                sqlite3_bind_parameter_index(
                    delete_all_site_entity_id_reuse_stmt,
                    "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "For delete_all_site_entity_id_reuse_stmt, could not bind $SITEID");
            }


            rc = sqlite3_step(delete_all_site_entity_id_reuse_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "Could not delete all site reuse entity IDs: "
                    + std::string(sqlite3_errstr(rc)));

                success = false;
            }

            if (sqlite3_bind_int(
                delete_site_next_entity_id_stmt,
                sqlite3_bind_parameter_index(
                    delete_site_next_entity_id_stmt,
                    "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "For delete_site_next_entity_id_stmt, could not bind $SITEID");
            }


            rc = sqlite3_step(delete_site_next_entity_id_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "Could not delete site fresh ID table entry: "
                    + std::string(sqlite3_errstr(rc)));

                success = false;
            }

            reset(mark_site_deleted_stmt);
            reset(delete_all_site_entity_id_reuse_stmt);
            reset(delete_site_next_entity_id_stmt);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::create_tables(void)
    {
        const std::string create_tables_str =
         "CREATE TABLE IF NOT EXISTS entities("
            "site_id INTEGER NOT NULL,"
            "entity_id INTEGER NOT NULL,"
            "owner INTEGER,"
            "type INTEGER NOT NULL,"
            "version INTEGER NOT NULL,"
            "name TEXT NOT NULL COLLATE NOCASE,"
            "data BLOB NOT NULL,"
         "PRIMARY KEY(site_id, entity_id)) WITHOUT ROWID;"
         "CREATE INDEX IF NOT EXISTS entity_type_idx ON entities(site_id, name, type);"

         "CREATE TABLE IF NOT EXISTS sites("
            "site_id INTEGER NOT NULL,"
            "deleted INTEGER NOT NULL,"
            "PRIMARY KEY(site_id, deleted)) WITHOUT ROWID;"

         "CREATE TABLE IF NOT EXISTS id_reuse("
            "site_id INTEGER NOT NULL,"
            "deleted_entity_id INTEGER NOT NULL,"
            "PRIMARY KEY(site_id, deleted_entity_id)) WITHOUT ROWID;"

         "CREATE TABLE IF NOT EXISTS next_id("
            "site_id INTEGER NOT NULL,"
            "next_entity_id INTEGER NOT NULL,"
            "PRIMARY KEY(site_id)) WITHOUT ROWID;"

         "CREATE TABLE IF NOT EXISTS next_site_id("
            "site_id INTEGER NOT NULL,"
            "PRIMARY KEY(site_id)) WITHOUT ROWID;"

         "CREATE TABLE IF NOT EXISTS display_names("
            "site_id INTEGER NOT NULL,"
            "entity_id INTEGER NOT NULL,"
            "player INTEGER NOT NULL,"
            "name TEXT NOT NULL COLLATE NOCASE,"
            "display_name TEXT NOT NULL COLLATE NOCASE,"
            "PRIMARY KEY(site_id, entity_id),"
            "FOREIGN KEY(site_id, entity_id) REFERENCES entities(site_id, entity_id)) WITHOUT ROWID;"
         "CREATE INDEX IF NOT EXISTS display_name_player_idx ON display_names(site_id, player, name, display_name);";

        bool success = true;
        char *rc_error_str_ptr = 0;
        const int rc = sqlite3_exec(
            dbhandle_ptr,
            create_tables_str.c_str(),
            0,
            0,
            &rc_error_str_ptr);

        if (rc != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "create_tables",
                "Unable to create tables: " + std::string(sqlite3_errstr(rc))
                + "   Full error: " + std::string(rc_error_str_ptr));

            sqlite3_free(rc_error_str_ptr);
            rc_error_str_ptr = 0;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::sql_init(void)
    {
        bool success = true;

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT site_id FROM sites WHERE deleted = 0;",
            -1,
            &list_sites_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for finding valid sites.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT site_id FROM sites WHERE deleted = 1;",
            -1,
            &list_deleted_sites_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for finding deleted sites.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID;",
            -1,
            &list_all_entities_site_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing all of site's entities.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
                "name LIKE '%' || $NAME || '%';",
            -1,
            &find_name_in_db_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by name.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
                "name LIKE '%' || $NAME || '%' AND type = $TYPE;",
            -1,
            &find_name_type_in_db_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by name and type.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
                "name = $NAME AND type = $TYPE;",
            -1,
            &find_exact_name_type_in_db_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by exact "
                    "name and type.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT type FROM entities WHERE site_id = $SITEID "
            "and entity_id = $ENTITYID;",
            -1,
            &get_entity_type_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting an Entity type.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE sites SET deleted = 0 WHERE site_id = $SITEID;",
            -1,
            &undelete_site_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for reusing a site.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT site_id FROM next_site_id;",
            -1,
            &next_site_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting a new site ID.");
        }

        // Starts at 2 because this is called after first site created.
        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO next_site_id(site_id) VALUES (2);",
            -1,
            &insert_first_next_site_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for inserting the first next site ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE next_site_id SET site_id = $SITEID;",
            -1,
            &update_next_site_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for updating the next site ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO sites(site_id, deleted) VALUES ($SITEID, 0);",
            -1,
            &insert_new_site_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for inserting a new site.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO next_id(site_id, next_entity_id) VALUES ($SITEID, 1);",
            -1,
            &insert_first_site_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for inserting first site ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM entities WHERE site_id = $SITEID;",
            -1,
            &delete_site_entities_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for delete a site's entities.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM display_names WHERE site_id = $SITEID;",
            -1,
            &delete_site_display_names_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for delete a site's display names.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE entities SET owner = $OWNER, type = $TYPE, name = $NAME, "
                                "data = $DATA "
                "WHERE site_id = $SITEID and entity_id = $ENTITYID;",
            -1,
            &update_entity_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for updating an Entity.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT type, data FROM entities WHERE site_id = $SITEID "
                "and entity_id = $ENTITYID;",
            -1,
            &get_entity_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting an Entity.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT deleted_entity_id FROM id_reuse WHERE site_id = $SITEID;",
            -1,
            &get_next_deleted_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for get next deleted entity ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM id_reuse WHERE site_id = $SITEID "
                "AND deleted_entity_id = $ENTITYID;",
            -1,
            &mark_deleted_id_used_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for marking reused entity ID as used.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT next_entity_id FROM next_id WHERE site_id = $SITEID;",
            -1,
            &get_next_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting next fresh entity ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE next_id SET next_entity_id = $NEXTID WHERE site_id = $SITEID;",
            -1,
            &update_next_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for updating next entity ID counter.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO "
              "entities(site_id, entity_id, owner, type, version, name, data) "
              "VALUES ($SITEID, $ENTITYID, $OWNER, $TYPE, $VERSION, $NAME, $DATA);",
            -1,
            &add_entity_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for inserting new Entity.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM entities WHERE site_id = $SITEID AND entity_id = $ENTITYID;",
            -1,
            &delete_entity_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for deleting an Entity.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO id_reuse(site_id, deleted_entity_id) VALUES "
                "($SITEID, $ENTITYID);",
            -1,
            &add_reuse_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for adding Entity to reuse table.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE sites SET deleted = 1 WHERE site_id = $SITEID;",
            -1,
            &mark_site_deleted_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for marking site as deleted.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM id_reuse WHERE site_id = $SITEID;",
            -1,
            &delete_all_site_entity_id_reuse_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for deleting site reuse Entity IDs.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM next_id WHERE site_id = $SITEID;",
            -1,
            &delete_site_next_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for deleting site next Entity ID.");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::delete_site_entity_data(
        const dbtype::Id::SiteIdType site_id)
    {
        bool success = true;
        int rc = SQLITE_OK;

        if (sqlite3_bind_int(
            delete_site_entities_stmt,
            sqlite3_bind_parameter_index(
                delete_site_entities_stmt,
                "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "delete_site_entity_data",
                "For delete_site_entities_stmt, could not bind $SITEID");
        }

        rc = sqlite3_step(delete_site_entities_stmt);

        if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "delete_site_entity_data",
                "Could not delete site entities: "
                + std::string(sqlite3_errstr(rc)));

            success = false;
        }
        else
        {
            rc = SQLITE_OK;
        }

        reset(delete_site_entities_stmt);

        if (sqlite3_bind_int(
            delete_site_display_names_stmt,
            sqlite3_bind_parameter_index(
                delete_site_display_names_stmt,
                "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "delete_site_entity_data",
                "For delete_site_display_names_stmt, could not bind $SITEID");
        }

        rc = sqlite3_step(delete_site_display_names_stmt);

        if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "delete_site_entity_data",
                "Could not delete site display names: "
                + std::string(sqlite3_errstr(rc)));

            success = false;
        }
        else
        {
            rc = SQLITE_OK;
        }

        reset(delete_site_display_names_stmt);

        return success;
    }

    // ----------------------------------------------------------------------
    void SqliteBackend::add_entity_ids(
        sqlite3_stmt *result_stmt_ptr,
        const dbtype::Id::SiteIdType site_id,
        dbtype::Entity::IdVector &result)
    {
        if (result_stmt_ptr)
        {
            int rc = sqlite3_step(result_stmt_ptr);

            while (rc == SQLITE_ROW)
            {
                result.push_back(
                    dbtype::Id(
                        site_id,
                        (dbtype::Id::EntityIdType)
                            sqlite3_column_int64(result_stmt_ptr, 0)));
                rc = sqlite3_step(result_stmt_ptr);
            }

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "add_entity_ids",
                    "Error getting list of entity IDs: "
                    + std::string(sqlite3_errstr(rc)));
            }
        }
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::bind_entity_update_params(
        dbtype::Entity *entity_ptr,
        concurrency::WriterLockToken &token,
        sqlite3_stmt *stmt)
    {
        bool success = entity_ptr;

        if (success)
        {
            utility::MemoryBuffer buffer;
            const std::string name = entity_ptr->get_entity_name(token);
            char *data_ptr = 0;
            size_t data_size = 0;
            success = serialize_entity(entity_ptr, buffer) and
                      buffer.get_data(data_ptr, data_size);

            if (not success)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "Could not serialize entity!");
            }

            if (sqlite3_bind_int64(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$OWNER"),
                entity_ptr->get_entity_owner(token).get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $OWNER");
            }

            if (sqlite3_bind_int(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$TYPE"),
                entity_ptr->get_entity_type()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $TYPE");
            }

            if (sqlite3_bind_text(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$NAME"),
                name.c_str(),
                name.size(),
                SQLITE_TRANSIENT) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $NAME");
            }

            if (sqlite3_bind_int(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$SITEID"),
                entity_ptr->get_entity_id().get_site_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$ENTITYID"),
                entity_ptr->get_entity_id().get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $ENTITYID");
            }

            if (sqlite3_bind_blob(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$DATA"),
                data_ptr,
                data_size,
                SQLITE_TRANSIENT) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $DATA");
            }
        }

        return success;

    }

    // ----------------------------------------------------------------------
    void SqliteBackend::reset(sqlite3_stmt *stmt_ptr)
    {
        if (stmt_ptr)
        {
            sqlite3_reset(stmt_ptr);
            sqlite3_clear_bindings(stmt_ptr);
        }
    }
}
}

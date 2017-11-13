/* === This file is part of Calamares - <http://github.com/calamares> ===
 *
 *   Copyright 2016, Lisa Vitolo <shainer@chakraos.org>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ChangeFilesystemLabelJob.h"

#include "utils/Logger.h"

#include <kpmcore/core/partition.h>
#include <kpmcore/backend/corebackend.h>
#include <kpmcore/backend/corebackenddevice.h>
#include <kpmcore/backend/corebackendmanager.h>
#include <kpmcore/backend/corebackendpartition.h>
#include <kpmcore/backend/corebackendpartitiontable.h>
#include <kpmcore/core/device.h>
#include <kpmcore/util/report.h>

ChangeFilesystemLabelJob::ChangeFilesystemLabelJob( Device* device,
                                            Partition* partition,
                                            const QString& newLabel )
    : PartitionJob( partition )
    , m_device( device )
    , m_label( newLabel )
{}


QString
ChangeFilesystemLabelJob::prettyName() const
{
    return tr( "Set filesystem label on %1." ).arg( partition()->partitionPath() );
}


QString
ChangeFilesystemLabelJob::prettyDescription() const
{
    return tr( "Set filesystem label <strong>%1</strong> to partition "
               "<strong>%2</strong>." )
            .arg( m_label )
            .arg( partition()->partitionPath() );
}


QString
ChangeFilesystemLabelJob::prettyStatusMessage() const
{
    return prettyDescription();
}


Calamares::JobResult
ChangeFilesystemLabelJob::exec()
{
    if (m_label == partition()->fileSystem().label()) {
        return Calamares::JobResult::ok();
    }

    Report report( nullptr );
    CoreBackend* backend = CoreBackendManager::self()->backend();

    QScopedPointer< CoreBackendDevice > backendDevice( backend->openDevice( m_device->deviceNode() ) );
    if ( !backendDevice.data() )
    {
        return Calamares::JobResult::error(
                   tr( "Could not open device '%1'." ).arg( m_device->deviceNode() ),
                   report.toText()
               );
    }

    QScopedPointer< CoreBackendPartitionTable > backendPartitionTable( backendDevice->openPartitionTable() );
    if ( !backendPartitionTable.data() )
    {
        return Calamares::JobResult::error(
                   tr( "Could not open partition table on device '%1'." ).arg( m_device->deviceNode() ),
                   report.toText()
               );
    }

    QScopedPointer< CoreBackendPartition > backendPartition(
            ( partition()->roles().has( PartitionRole::Extended ) )
            ? backendPartitionTable->getExtendedPartition()
            : backendPartitionTable->getPartitionBySector( partition()->firstSector() )
    );
    if ( !backendPartition.data() ) {
        return Calamares::JobResult::error(
                   tr( "Could not find partition '%1'." ).arg( partition()->partitionPath() ),
                   report.toText()
               );
    }

    FileSystem& fs = m_partition->fileSystem();
    fs.setLabel( m_label );

    if ( !backendPartitionTable->setPartitionSystemType( report, *m_partition ) )
    {
        return Calamares::JobResult::error(
                   tr( "The installer failed to update partition table on disk '%1'." ).arg( m_device->name() ),
                   report.toText()
               );
    }

    backendPartitionTable->commit();
    return Calamares::JobResult::ok();
}


#include "ChangeFilesystemLabelJob.moc"

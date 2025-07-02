import { Injectable, OnModuleInit, Logger } from '@nestjs/common';
import { DataSource } from 'typeorm';

@Injectable()
export class DbLoggerService implements OnModuleInit {
  private readonly logger = new Logger(DbLoggerService.name);

  constructor(private readonly dataSource: DataSource) {}

  async onModuleInit() {
    try {
      await this.dataSource.query('SELECT 1');
      this.logger.log('✅ Successfully connected to TimescaleDB');
    } catch (error) {
      this.logger.error('❌ Failed to connect to TimescaleDB', error);
    }
  }
}
